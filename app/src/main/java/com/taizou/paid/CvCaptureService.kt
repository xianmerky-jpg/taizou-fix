package com.taizou.paid

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.Context
import android.content.Intent
import android.graphics.PixelFormat
import android.hardware.display.DisplayManager
import android.hardware.display.VirtualDisplay
import android.media.ImageReader
import android.media.projection.MediaProjection
import android.media.projection.MediaProjectionManager
import android.os.Build
import android.os.Handler
import android.os.HandlerThread
import android.os.IBinder
import android.os.Process
import android.util.DisplayMetrics
import android.util.Log
import android.view.WindowManager

/**
 * Screen-capture source for CV ESP (wallhack-red blob tracking).
 *
 * Flow: MainActivity obtains MediaProjection consent, then starts this
 * foreground service (type mediaProjection) with the result. The service
 * captures a downscaled RGBA stream, runs the native red-blob detector at
 * ~8fps and reports boxes through [listener]. A blackout detector stops the
 * session with a message when frames come back black (e.g. FLAG_SECURE).
 *
 * Our own overlay draws green/white only, so self-capture can never
 * retrigger the red detector (no feedback loop).
 */
class CvCaptureService : Service() {

    data class CvBox(val x1: Float, val y1: Float, val x2: Float, val y2: Float)

    interface CvListener {
        fun onBoxes(boxes: List<CvBox>, captureW: Int, captureH: Int, fresh: Boolean, frameMs: Long)
        fun onStopped()
        fun onBlocked(message: String)
    }

    companion object {
        const val ACTION_START = "cv_start"
        const val ACTION_STOP = "cv_stop"
        const val EXTRA_RESULT_CODE = "resultCode"
        const val EXTRA_DATA = "data"
        const val CHANNEL_ID = "cv_capture"
        const val NOTIF_ID = 4101

        @Volatile
        var running = false
        var listener: CvListener? = null
    }

    private var projection: MediaProjection? = null
    private var virtualDisplay: VirtualDisplay? = null
    private var imageReader: ImageReader? = null
    private var worker: HandlerThread? = null
    private var workerHandler: Handler? = null
    private var captureW: Int = 0
    private var captureH: Int = 0
    private var lastProcessMs: Long = 0
    private var lastProbeMs: Long = 0
    private var blackStreak: Int = 0
    private val pixelsLock = Any()
    private var pixels: IntArray = IntArray(0)

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        when (intent?.action) {
            ACTION_STOP -> {
                teardown()
                stopSelf()
                return START_NOT_STICKY
            }
            else -> {
                val code = intent?.getIntExtra(EXTRA_RESULT_CODE, 0) ?: 0
                val data: Intent? = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                    intent?.getParcelableExtra(EXTRA_DATA, Intent::class.java)
                } else {
                    @Suppress("DEPRECATION")
                    intent?.getParcelableExtra(EXTRA_DATA)
                }
                if (code == 0 || data == null) {
                    Log.e("CvCapture", "missing consent result")
                    stopSelf()
                    return START_NOT_STICKY
                }
                startCapture(code, data)
                return START_STICKY
            }
        }
    }

    private fun startCapture(resultCode: Int, data: Intent) {
        try {
            ensureChannel()
            val notif = Notification.Builder(this, CHANNEL_ID)
                .setContentTitle("Taizou ESP capture running")
                .setContentText("Screen analysis for ESP overlay")
                .setSmallIcon(android.R.drawable.presence_video_online)
                .setOngoing(true)
                .build()
            startForeground(
                NOTIF_ID, notif,
                android.content.pm.ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PROJECTION
            )
        } catch (e: Exception) {
            Log.e("CvCapture", "startForeground failed", e)
            stopSelf()
            return
        }

        try {
            val wm = getSystemService(Context.WINDOW_SERVICE) as WindowManager
            val metrics = DisplayMetrics()
            @Suppress("DEPRECATION")
            wm.defaultDisplay.getRealMetrics(metrics)
            val dw = maxOf(metrics.widthPixels, metrics.heightPixels)
            val dh = minOf(metrics.widthPixels, metrics.heightPixels)
            // Downscale ~4x at the source (GPU does it free); keep aspect.
            val s = 600f / maxOf(dw, dh).toFloat()
            captureW = maxOf(160, (dw * s).toInt())
            captureH = maxOf(120, (dh * s).toInt())

            val mpm = getSystemService(Context.MEDIA_PROJECTION_SERVICE) as MediaProjectionManager
            val mp = mpm.getMediaProjection(resultCode, data)
            projection = mp

            worker = HandlerThread("CvCapture", Process.THREAD_PRIORITY_DEFAULT).apply { start() }
            workerHandler = Handler(worker!!.looper)

            mp.registerCallback(object : MediaProjection.Callback() {
                override fun onStop() {
                    teardown()
                    listener?.onStopped()
                    stopSelf()
                }
            }, workerHandler)

            val reader = ImageReader.newInstance(captureW, captureH, PixelFormat.RGBA_8888, 3)
            reader.setOnImageAvailableListener({ r ->
                var image: android.media.Image? = null
                try {
                    image = r.acquireLatestImage() ?: return@setOnImageAvailableListener
                    val acquireMs = android.os.SystemClock.uptimeMillis()
                    processImage(image, acquireMs)
                } catch (e: Exception) {
                    Log.e("CvCapture", "frame failed", e)
                } finally {
                    try {
                        image?.close()
                    } catch (e: Exception) {
                    }
                }
            }, workerHandler)
            imageReader = reader

            virtualDisplay = mp.createVirtualDisplay(
                "CvCapture", captureW, captureH, metrics.densityDpi,
                DisplayManager.VIRTUAL_DISPLAY_FLAG_AUTO_MIRROR,
                reader.surface, null, workerHandler
            )
            running = true
            Log.i("CvCapture", "capture started ${captureW}x${captureH}")
        } catch (e: Exception) {
            Log.e("CvCapture", "startCapture failed", e)
            listener?.onBlocked("Capture failed: ${e.message}")
            teardown()
            stopSelf()
        }
    }

    private fun processImage(image: android.media.Image, acquireMs: Long) {
        val now = android.os.SystemClock.uptimeMillis()
        if (now - lastProcessMs < 60) return  // ~16.7fps throttle
        lastProcessMs = now

        val plane = image.planes[0]
        val buf = plane.buffer
        val rowStride = plane.rowStride
        val pixelStride = plane.pixelStride
        val w = captureW
        val h = captureH
        synchronized(pixelsLock) {
            if (pixels.size != w * h) pixels = IntArray(w * h)
            buf.rewind()
            if (pixelStride == 4 && rowStride == w * 4) {
                buf.asIntBuffer().get(pixels, 0, w * h)
            } else {
                // Strided fallback.
                val row = ByteArray(rowStride)
                var dst = 0
                for (y in 0 until h) {
                    buf.get(row, 0, rowStride)
                    var p = 0
                    for (x in 0 until w) {
                        val r = row[p].toInt() and 0xFF
                        val g = row[p + 1].toInt() and 0xFF
                        val b = row[p + 2].toInt() and 0xFF
                        val a = row[p + 3].toInt() and 0xFF
                        pixels[dst++] = (a shl 24) or (b shl 16) or (g shl 8) or r
                        p += pixelStride
                    }
                }
            }
            // Periodic blackout probe (FLAG_SECURE would yield black frames),
            // wall-clock based so it survives cadence changes.
            if (now - lastProbeMs >= 2000) {
                lastProbeMs = now
                var sum = 0L
                var n = 0
                var i = 0
                while (i < pixels.size) {
                    val v = pixels[i]
                    sum += (v and 0xFF) + ((v shr 8) and 0xFF) + ((v shr 16) and 0xFF)
                    n += 3
                    i += 16
                }
                if (n > 0 && sum / n < 3) {
                    blackStreak++
                    if (blackStreak >= 3) {
                        listener?.onBlocked("Screen capture is blocked (black frames)")
                        teardown()
                        stopSelf()
                        return
                    }
                } else {
                    blackStreak = 0
                }
            }

            val out = try {
                TaizouNative.detectBoxes(pixels, w, h)
            } catch (e: Exception) {
                Log.e("CvCapture", "detect failed", e)
                return
            }
            if (out.isEmpty()) return
            val n = out[0].toInt().coerceIn(0, 64)
            val boxes = ArrayList<CvBox>(n)
            var o = 1
            for (k in 0 until n) {
                if (o + 3 >= out.size) break
                boxes.add(CvBox(out[o], out[o + 1], out[o + 2], out[o + 3]))
                o += 4
            }
            listener?.onBoxes(boxes, w, h, true, acquireMs)
        }
    }

    private fun ensureChannel() {
        val nm = getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        if (nm.getNotificationChannel(CHANNEL_ID) == null) {
            nm.createNotificationChannel(
                NotificationChannel(CHANNEL_ID, "ESP capture", NotificationManager.IMPORTANCE_LOW)
            )
        }
    }

    private fun teardown() {
        running = false
        try {
            virtualDisplay?.release()
        } catch (e: Exception) {
        }
        try {
            imageReader?.close()
        } catch (e: Exception) {
        }
        try {
            projection?.stop()
        } catch (e: Exception) {
        }
        try {
            worker?.quitSafely()
        } catch (e: Exception) {
        }
        virtualDisplay = null
        imageReader = null
        projection = null
        worker = null
        workerHandler = null
        try {
            stopForeground(STOP_FOREGROUND_REMOVE)
        } catch (e: Exception) {
        }
    }

    override fun onDestroy() {
        teardown()
        listener = null
        super.onDestroy()
    }
}
