package com.taizou.paid

import kotlin.math.hypot
import kotlin.math.max
import kotlin.math.min

/**
 * Frame-to-frame tracker for CV detection boxes (capture coordinates),
 * with velocity estimation for draw-time motion prediction.
 *
 * - Associate on SMOOTHED measured positions (never predicted ones):
 *   greedy highest-IoU-first (IoU > 0.2), else center-distance fallback
 *   (< 0.15 x track diagonal) for split boxes whose IoU collapsed.
 * - Confirm after 2 hits (one frame of appearance latency).
 * - Hold confirmed tracks up to 4 missed frames (bridges occlusion/animation
 *   dropout), then drop. Tentative tracks die after 1 miss.
 * - Smooth corners with EMA alpha 0.55; estimate velocity (px/ms) with
 *   deadzone + beta 0.5 + reversal guard + vmax clamp. Teleports snap.
 *
 * Prediction itself happens in EspOverlayView.onDraw (extrapolate by
 * velocity x (now - tMs)); this class only outputs (box, velocity, time).
 * Stateless detector stays pure in native code; all memory lives here.
 * Call [clear] on stream start/stop and on capture-size change.
 */
class CvTracker {

    data class Box(val x1: Float, val y1: Float, val x2: Float, val y2: Float)

    data class Tracked(
        val box: Box,
        val vx: Float,
        val vy: Float,
        val tMs: Long
    )

    private data class Track(
        var x1: Float, var y1: Float, var x2: Float, var y2: Float,
        var vx: Float = 0f,
        var vy: Float = 0f,
        var px: Float = 0f,
        var py: Float = 0f,
        var ptMs: Long = 0L,
        var tMs: Long = 0L,
        var hits: Int = 0,
        var misses: Int = 0,
        var confirmed: Boolean = false
    )

    companion object {
        private const val ALPHA = 0.55f
        private const val BETA = 0.5f
        private const val JITTER_FLOOR = 0.75f   // capture px
        private const val REVERSE_FLOOR = 0.15f  // capture px/ms
        private const val VMAX = 3.0f            // capture px/ms
    }

    private val tracks = mutableListOf<Track>()

    fun update(detected: List<Box>, nowMs: Long): List<Tracked> {
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
                // Teleport / target-switch snap before smoothing.
                val scx = (t.x1 + t.x2) / 2f
                val scy = (t.y1 + t.y2) / 2f
                val dcx = (d.x1 + d.x2) / 2f
                val dcy = (d.y1 + d.y2) / 2f
                val moveDist = hypot(dcx - scx, dcy - scy)
                val diag = hypot(t.x2 - t.x1, t.y2 - t.y1)
                if (moveDist > max(24f, 0.6f * diag)) {
                    t.x1 = d.x1
                    t.y1 = d.y1
                    t.x2 = d.x2
                    t.y2 = d.y2
                    t.vx = 0f
                    t.vy = 0f
                    t.px = dcx
                    t.py = dcy
                    t.ptMs = nowMs
                } else {
                    t.x1 += (d.x1 - t.x1) * ALPHA
                    t.y1 += (d.y1 - t.y1) * ALPHA
                    t.x2 += (d.x2 - t.x2) * ALPHA
                    t.y2 += (d.y2 - t.y2) * ALPHA
                    val cx = (t.x1 + t.x2) / 2f
                    val cy = (t.y1 + t.y2) / 2f
                    val dt = max(nowMs - t.ptMs, 1L).toFloat()
                    var ivx = (cx - t.px) / dt
                    var ivy = (cy - t.py) / dt
                    if (hypot(cx - t.px, cy - t.py) < JITTER_FLOOR) {
                        ivx = 0f
                        ivy = 0f
                    }
                    if (dt > 150) {
                        t.vx *= 0.5f
                        t.vy *= 0.5f
                    }
                    val dot = ivx * t.vx + ivy * t.vy
                    if (dot < 0 && hypot(ivx, ivy) > REVERSE_FLOOR) {
                        t.vx = ivx * 0.5f
                        t.vy = ivy * 0.5f
                    } else {
                        t.vx += (ivx - t.vx) * BETA
                        t.vy += (ivy - t.vy) * BETA
                    }
                    val sp = hypot(t.vx, t.vy)
                    if (sp > VMAX) {
                        t.vx *= VMAX / sp
                        t.vy *= VMAX / sp
                    }
                    t.px = cx
                    t.py = cy
                    t.ptMs = nowMs
                }
                t.hits++
                t.misses = 0
                if (!t.confirmed && t.hits >= 2) t.confirmed = true
                t.tMs = nowMs
            } else {
                val cx = (d.x1 + d.x2) / 2f
                val cy = (d.y1 + d.y2) / 2f
                tracks.add(
                    Track(d.x1, d.y1, d.x2, d.y2, 0f, 0f, cx, cy, nowMs, nowMs, hits = 1)
                )
            }
        }
        for (ti in tracks.indices) {
            if (!usedT[ti]) {
                val t = tracks[ti]
                t.misses++
                t.vx *= 0.85f
                t.vy *= 0.85f
            }
        }
        tracks.removeAll { (!it.confirmed && it.misses >= 2) || (it.confirmed && it.misses > 4) }
        while (tracks.size > 96) {
            val idx = tracks.indexOfFirst { !it.confirmed }
            if (idx < 0) break
            tracks.removeAt(idx)
        }

        val out = ArrayList<Tracked>(tracks.size)
        for (t in tracks) {
            if (t.confirmed) {
                out.add(Tracked(Box(t.x1, t.y1, t.x2, t.y2), t.vx, t.vy, t.tMs))
            }
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
