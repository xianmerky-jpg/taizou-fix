package com.taizou.paid

import kotlin.math.hypot
import kotlin.math.max
import kotlin.math.min

/**
 * Frame-to-frame tracker for CV detection boxes (capture coordinates).
 *
 * Kills the two dominant jitter sources: 1-frame noise (hitmarkers, muzzle
 * flashes, compression speckle) never draws, and corner positions are
 * exponentially smoothed so boxes stop stepping pixel-to-pixel.
 *
 * - Associate: greedy highest-IoU-first (IoU > 0.2), else center-distance
 *   fallback (< 0.15 x track diagonal) for split boxes whose IoU collapsed.
 * - Confirm after 2 hits (one frame of appearance latency).
 * - Hold confirmed tracks up to 4 missed frames (bridges occlusion/animation
 *   dropout), then drop. Tentative tracks die after 1 miss.
 * - Smooth corners with EMA alpha 0.35 in capture coords (scale after).
 *
 * Stateless detector stays pure in native code; all memory lives here.
 * Call [clear] on stream start/stop and on capture-size change.
 */
class CvTracker {

    data class Box(val x1: Float, val y1: Float, val x2: Float, val y2: Float)

    private data class Track(
        var x1: Float, var y1: Float, var x2: Float, var y2: Float,
        var hits: Int = 1,
        var misses: Int = 0,
        var confirmed: Boolean = false
    )

    private val tracks = mutableListOf<Track>()

    fun update(detected: List<Box>): List<Box> {
        // Greedy highest-IoU-first association (biggest first for stability).
        val order = detected.indices.sortedByDescending { i ->
            val b = detected[i]
            max(0f, b.x2 - b.x1) * max(0f, b.y2 - b.y1)
        }
        val usedT = BooleanArray(tracks.size)
        val matched = arrayOfNulls<Track>(detected.size)
        for (di in order) {
            val d = detected[di]
            var bi = -1
            var best = 0.2f
            for (ti in tracks.indices) {
                if (usedT[ti]) continue
                val iou = iou(d, tracks[ti])
                if (iou > best) {
                    best = iou
                    bi = ti
                }
            }
            if (bi < 0) {
                var bd = Float.MAX_VALUE
                for (ti in tracks.indices) {
                    if (usedT[ti]) continue
                    val t = tracks[ti]
                    val dx = (d.x1 + d.x2) / 2f - (t.x1 + t.x2) / 2f
                    val dy = (d.y1 + d.y2) / 2f - (t.y1 + t.y2) / 2f
                    val diag = hypot(t.x2 - t.x1, t.y2 - t.y1)
                    val dist = hypot(dx, dy)
                    if (diag > 1f && dist < 0.15f * diag && dist < bd) {
                        bd = dist
                        bi = ti
                    }
                }
            }
            if (bi >= 0) {
                usedT[bi] = true
                matched[di] = tracks[bi]
            }
        }

        for (di in detected.indices) {
            val t = matched[di]
            val d = detected[di]
            if (t != null) {
                t.hits++
                t.misses = 0
                if (!t.confirmed && t.hits >= 2) t.confirmed = true
                val a = 0.35f
                t.x1 += (d.x1 - t.x1) * a
                t.y1 += (d.y1 - t.y1) * a
                t.x2 += (d.x2 - t.x2) * a
                t.y2 += (d.y2 - t.y2) * a
            } else {
                tracks.add(Track(d.x1, d.y1, d.x2, d.y2))
            }
        }
        for (ti in tracks.indices) {
            if (!usedT[ti]) tracks[ti].misses++
        }
        tracks.removeAll { (!it.confirmed && it.misses >= 2) || (it.confirmed && it.misses > 4) }
        while (tracks.size > 96) {
            val idx = tracks.indexOfFirst { !it.confirmed }
            if (idx < 0) break
            tracks.removeAt(idx)
        }

        val out = ArrayList<Box>(tracks.size)
        for (t in tracks) {
            if (t.confirmed) out.add(Box(t.x1, t.y1, t.x2, t.y2))
        }
        return out
    }

    fun clear() {
        tracks.clear()
    }

    private fun iou(d: Box, t: Track): Float {
        val ix1 = max(d.x1, t.x1)
        val iy1 = max(d.y1, t.y1)
        val ix2 = min(d.x2, t.x2)
        val iy2 = min(d.y2, t.y2)
        val iw = ix2 - ix1
        val ih = iy2 - iy1
        if (iw <= 0 || ih <= 0) return 0f
        val inter = iw * ih
        val union = (d.x2 - d.x1) * (d.y2 - d.y1) + (t.x2 - t.x1) * (t.y2 - t.y1) - inter
        return if (union <= 0) 0f else inter / union
    }
}
