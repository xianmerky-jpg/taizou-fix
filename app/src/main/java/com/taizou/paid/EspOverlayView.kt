package com.taizou.paid

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.util.AttributeSet
import android.view.View

/**
 * Full-screen transparent ESP layer. Drawn entirely with Canvas primitives,
 * following the same pattern as the clock/ECG views. Frames are pushed from
 * MainActivity (which polls the native reader on a background thread);
 * this view never touches JNI itself.
 *
 * Geometry mirrors the reference draw spec: Bottom-anchored lines, Outline
 * boxes (w = h * 0.65), name/health-top containers, distance containers,
 * 15-segment skeletons, count header. Scale = display density.
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

    private val strokePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.STROKE
        strokeWidth = 2f
    }
    private val fillPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.FILL
    }
    private val textPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = Color.WHITE
        textAlign = Paint.Align.CENTER
    }
    private val shadowPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = Color.BLACK
        textAlign = Paint.Align.CENTER
    }

    fun setFrame(newItems: List<Item>, enemies: Int, bots: Int) {
        items = newItems
        totalEnemies = enemies
        totalBots = bots
        invalidate()
    }

    fun clearFrame() {
        items = emptyList()
        totalEnemies = 0
        totalBots = 0
        invalidate()
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        val w = width.toFloat()
        val h = height.toFloat()
        if (w <= 0 || h <= 0) return
        val s = resources.displayMetrics.density

        if ("esp_count" in enabled) drawCount(canvas, w)
        for (it in items) {
            if (!it.projected) continue
            val col = if (it.isBot) Color.argb(180, 0, 255, 0) else Color.argb(255, 255, 0, 0)
            if ("esp_line" in enabled) {
                // Bottom anchor like the reference default.
                canvas.drawLine(w / 2f, h, it.headX, it.headY, strokePaint.apply { color = col })
            }
            if ("esp_box" in enabled) {
                canvas.drawRect(
                    it.rootX - it.boxW / 2f, it.headY,
                    it.rootX + it.boxW / 2f, it.headY + it.boxH,
                    strokePaint.apply { color = col }
                )
            }
            if ("esp_skeleton" in enabled) drawSkeleton(canvas, it, col)
            // Name/distance/health-top share the <=60m gate. Screen coords from
            // native are already top-left origin (no extra flip).
            if (it.dist in 0f..60f) {
                if ("esp_name" in enabled) drawName(canvas, it, s)
                if ("esp_distance" in enabled) drawDistance(canvas, it, s)
                if ("esp_health" in enabled) drawHealthTop(canvas, it, s)
            }
        }
    }

    private fun drawSkeleton(canvas: Canvas, it: Item, col: Int) {
        val paint = strokePaint.apply { color = col }
        for ((a, b) in skeletonEdges) {
            if (a >= it.bones.size || b >= it.bones.size) continue
            val ba = it.bones[a]
            val bb = it.bones[b]
            if (!ba.visible || !bb.visible) continue
            canvas.drawLine(ba.x, ba.y, bb.x, bb.y, paint)
        }
    }

    private fun drawName(canvas: Canvas, it: Item, s: Float) {
        val hasHealth = "esp_health" in enabled
        val cw = it.boxW * 1.6f * s
        val cx = it.headX - cw / 2f
        val cy = it.headY - 50f * s
        val ch = 24f * s + (if (hasHealth) 10f * s else 0f)
        canvas.drawRect(cx, cy, cx + cw, cy + ch, fillPaint.apply { color = Color.argb(120, 0, 0, 0) })
        textPaint.textSize = 18f * s
        canvas.drawText(it.name, cx + cw / 2f, cy + 20f * s, textPaint)
    }

    private fun drawHealthTop(canvas: Canvas, it: Item, s: Float) {
        val cw = it.boxW * 1.6f * s
        val cx = it.headX - cw / 2f
        val cy = it.headY - 50f * s
        val max = if (it.maxHp > 0) it.maxHp else 100f
        val ratio = (it.hp / max).coerceIn(0f, 1f)
        val pad = 5f * s
        val barH = 7f * s
        // Bar sits at the bottom of the name container.
        val bx = cx + pad
        val by = cy + 24f * s + 10f * s - barH - 2f * s
        val bw = cw - 2f * pad
        val r = (510f * (1f - ratio)).coerceIn(0f, 255f).toInt()
        val g = (510f * ratio).coerceIn(0f, 255f).toInt()
        canvas.drawRect(bx, by, bx + bw, by + barH, fillPaint.apply { color = Color.BLACK })
        canvas.drawRect(bx, by, bx + bw * ratio, by + barH, fillPaint.apply { color = Color.rgb(r, g, 0) })
    }

    private fun drawDistance(canvas: Canvas, it: Item, s: Float) {
        val label = "${it.dist.toInt()}m"
        val scale = when {
            it.dist >= 19f -> 1.3f
            it.dist >= 17f -> 1.2f
            it.dist >= 15f -> 1.1f
            else -> 1.0f
        }
        textPaint.textSize = 15f * resources.displayMetrics.scaledDensity
        val tw = textPaint.measureText(label)
        val cw = tw + 8f * s
        val cx = it.headX - cw / 2f
        val cy = it.rootY + 8f * s
        val ch = textPaint.textSize + 8f * s
        canvas.drawRect(cx, cy, cx + cw, cy + ch, fillPaint.apply { color = Color.argb(120, 0, 0, 0) })
        canvas.drawText(label, cx + cw / 2f, cy + 4f * s + textPaint.textSize, textPaint)
    }

    private fun drawCount(canvas: Canvas, w: Float) {
        val total = totalEnemies + totalBots
        textPaint.textSize = 15f * resources.displayMetrics.scaledDensity
        if (total > 0) {
            val pLabel = "Player: $totalEnemies"
            val bLabel = "Bot: $totalBots"
            val pw = textPaint.measureText(pLabel)
            val bw = textPaint.measureText(bLabel)
            val totalW = pw + 40f + bw
            var x = (w - totalW) / 2f
            val y = 80f
            drawShadowText(canvas, pLabel, x + pw / 2f, y, Color.argb(255, 255, 80, 80))
            x += pw + 40f
            drawShadowText(canvas, bLabel, x + bw / 2f, y, Color.argb(255, 80, 255, 80))
        } else {
            val label = "[ SAFE ]"
            val lw = textPaint.measureText(label)
            drawShadowText(canvas, label, (w - lw) / 2f + lw / 2f, 80f, Color.argb(255, 0, 255, 0))
        }
    }

    private fun drawShadowText(canvas: Canvas, text: String, x: Float, y: Float, col: Int) {
        shadowPaint.textSize = textPaint.textSize
        for ((dx, dy) in arrayOf(-1f to -1f, 1f to -1f, -1f to 1f, 1f to 1f)) {
            canvas.drawText(text, x + dx, y + dy, shadowPaint)
        }
        canvas.drawText(text, x, y, textPaint.apply { color = col })
    }
}
