package com.taizou.paid

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.util.AttributeSet
import android.view.View
import kotlin.math.abs

/**
 * Full-screen transparent ESP layer. Drawn entirely with Canvas primitives.
 * Frames are pushed from MainActivity (which polls the native reader on a
 * background thread); this view never touches JNI itself.
 *
 * Rendering mirrors the proven reference module 1:1: corner boxes sized from
 * skeleton bounds (else head/root), side HP bar, name plate, plain distance
 * text, Top-anchored lines, 15-segment skeletons + head circle, HUD count
 * bar with fresh/stale states. Player green, bot white. Raw px metrics.
 */
class EspOverlayView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr) {

    data class Bone(val x: Float, val y: Float, val visible: Boolean)

    data class Item(
        val headX: Float, val headY: Float,
        val rootX: Float, val rootY: Float,
        val boxW: Float, val boxH: Float,
        val dist: Float,
        val hp: Float, val maxHp: Float,
        val isBot: Boolean,
        val projected: Boolean,
        val name: String,
        val bones: List<Bone>
    )

    // Skeleton joint pairs (reference order).
    private val skeletonEdges = arrayOf(
        0 to 1, 1 to 2, 2 to 3, 1 to 4, 4 to 5, 5 to 6,
        1 to 7, 7 to 8, 8 to 9, 3 to 10, 10 to 11, 11 to 12,
        3 to 13, 13 to 14, 14 to 15
    )

    var enabled: Set<String> = emptySet()

    private var items: List<Item> = emptyList()
    private var totalEnemies: Int = 0
    private var totalBots: Int = 0
    private var fresh: Boolean = false

    private val skeletonPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.STROKE
        strokeWidth = 2.3f
        strokeCap = Paint.Cap.ROUND
    }
    private val boxPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.STROKE
        strokeWidth = 1.5f
    }
    private val hpBgPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.FILL
        color = Color.argb(0xBB, 0, 0, 0)
    }
    private val hpBarPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.FILL
    }
    private val nameBgPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.FILL
        color = Color.argb(0x96, 0, 0, 0)
    }
    private val namePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = Color.WHITE
        textSize = 14f
        isFakeBoldText = true
    }
    private val distPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = Color.WHITE
        textSize = 12f
    }
    private val headCirclePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.STROKE
        strokeWidth = 2f
    }
    private val hudBgPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.FILL
    }
    private val countPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = Color.WHITE
        textAlign = Paint.Align.CENTER
        textSize = 25f
        isFakeBoldText = true
    }
    private val linePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.STROKE
        strokeWidth = 1.5f
        color = Color.argb(0xC8, 0xFF, 0xFF, 0xFF)
    }

    fun setFrame(newItems: List<Item>, enemies: Int, bots: Int, isFresh: Boolean) {
        items = newItems
        totalEnemies = enemies
        totalBots = bots
        fresh = isFresh
        invalidate()
    }

    fun clearFrame() {
        items = emptyList()
        totalEnemies = 0
        totalBots = 0
        fresh = false
        invalidate()
    }

    private fun hpColor(ratio: Float): Int {
        return if (ratio < 0.5f) {
            val g = (255f * (ratio / 0.5f)).toInt()
            Color.rgb(255, g, 0)
        } else {
            val r = (255f * (1f - (ratio - 0.5f) / 0.5f)).toInt()
            Color.rgb(r, 255, 0)
        }
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        val w = width.toFloat()
        val h = height.toFloat()
        if (w <= 0 || h <= 0) return

        if ("esp_count" in enabled) drawCount(canvas, w)

        for (it in items) {
            if (!it.projected) continue
            val col = if (it.isBot) Color.WHITE else Color.rgb(0, 255, 0)

            // Anchor on skeleton bounds when available, else head/root.
            var useHeadX = it.headX
            var useHeadY = it.headY
            var useFootY = it.rootY
            var minY = Float.MAX_VALUE
            var maxY = -Float.MAX_VALUE
            var minX = 0f
            var hasSkel = false
            for (b in it.bones) {
                if (!b.visible) continue
                hasSkel = true
                if (b.y < minY) { minY = b.y; minX = b.x }
                if (b.y > maxY) { maxY = b.y }
            }
            if (hasSkel) {
                useHeadX = minX
                useHeadY = minY
                useFootY = maxY
            }

            val boxH = if (hasSkel) maxOf(useFootY - useHeadY, 4f) else it.boxH.coerceIn(4f, h)
            val boxW = if (hasSkel) boxH * 0.65f else it.boxW.coerceIn(4f, w)
            val left = useHeadX - boxW / 2f
            val right = left + boxW

            if ("esp_skeleton" in enabled) {
                for ((a, b) in skeletonEdges) {
                    if (a >= it.bones.size || b >= it.bones.size) continue
                    val ba = it.bones[a]
                    val bb = it.bones[b]
                    if (!ba.visible || !bb.visible) continue
                    canvas.drawLine(ba.x, ba.y, bb.x, bb.y, skeletonPaint.apply { color = col })
                }
                val rad = boxH / 9.3f
                canvas.drawCircle(
                    useHeadX, useHeadY - boxH * 0.085f, rad,
                    headCirclePaint.apply { color = col }
                )
            }

            if ("esp_box" in enabled) {
                val cw = boxW / 4f
                val ch = boxH / 4f
                boxPaint.color = col
                canvas.drawLine(left, useHeadY, left + cw, useHeadY, boxPaint)
                canvas.drawLine(left, useHeadY, left, useHeadY + ch, boxPaint)
                canvas.drawLine(right - cw, useHeadY, right, useHeadY, boxPaint)
                canvas.drawLine(right, useHeadY, right, useHeadY + ch, boxPaint)
                canvas.drawLine(left, useFootY, left + cw, useFootY, boxPaint)
                canvas.drawLine(left, useFootY, left, useFootY - ch, boxPaint)
                canvas.drawLine(right - cw, useFootY, right, useFootY, boxPaint)
                canvas.drawLine(right, useFootY, right, useFootY - ch, boxPaint)
            }

            if ("esp_health" in enabled && it.maxHp > 0) {
                val ratio = (it.hp / it.maxHp).coerceIn(0f, 1f)
                val barX = left - 7f
                val barW = 4f
                canvas.drawRect(barX, useHeadY, barX + barW, useFootY, hpBgPaint)
                hpBarPaint.color = hpColor(ratio)
                val fillH = boxH * (1f - ratio)
                canvas.drawRect(barX, useHeadY + fillH, barX + barW, useFootY, hpBarPaint)
            }

            if ("esp_name" in enabled && it.name != "") {
                val tw = namePaint.measureText(it.name)
                val nx = useHeadX - tw / 2f
                val ny = useHeadY - 20f
                canvas.drawRect(nx - 2f, ny - 14f, nx + tw + 2f, ny + 2f, nameBgPaint)
                canvas.drawText(it.name, nx, ny, namePaint)
            }

            if ("esp_distance" in enabled) {
                val txt = "${it.dist.toInt()}M"
                val tw = distPaint.measureText(txt)
                canvas.drawText(txt, useHeadX - tw / 2f, useFootY + 20f, distPaint)
            }

            if ("esp_line" in enabled) {
                // Top style (reference default): from top-center, ending
                // above the head (higher when the name plate is on).
                val startX = w / 2f
                val startY = 120f
                var endX = useHeadX
                var endY = useHeadY
                val margin = 10f
                if ("esp_name" in enabled && it.name != "") {
                    endY = useHeadY - 40f - margin
                } else {
                    endY = useHeadY - margin
                }
                if (endX == endX && endY == endY &&
                    abs(endX) < 15000 && abs(endY) < 15000
                ) {
                    canvas.drawLine(startX, startY, endX, endY, linePaint.apply { color = col })
                }
            }
        }
    }

    private fun drawCount(canvas: Canvas, w: Float) {
        val total = totalEnemies + totalBots
        hudBgPaint.color = when {
            !fresh -> Color.argb(0x88, 0xFF, 0, 0)
            total == 0 -> Color.argb(0x88, 0x07, 0xC9, 0x1A)
            else -> Color.argb(0x85, 0xFF, 0x5F, 0)
        }
        canvas.drawRect(w / 2f - 120f, 55f, w / 2f + 130f, 100f, hudBgPaint)
        val info = if (!fresh || total == 0) {
            "CLEAR"
        } else {
            "Bots: $totalBots | Players: $totalEnemies"
        }
        canvas.drawText(info, w / 2f, 87.5f, countPaint)
    }
}
