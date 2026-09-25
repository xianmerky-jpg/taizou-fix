package com.taizou.paid

import android.app.Service
import android.content.Context
import android.content.Intent
import android.graphics.PixelFormat
import android.os.Build
import android.os.IBinder
import android.view.Gravity
import android.view.MotionEvent
import android.view.View
import android.view.WindowManager
import android.widget.FrameLayout
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleObserver
import androidx.lifecycle.LifecycleOwner
import androidx.lifecycle.OnLifecycleEvent

class OverlayService : Service(), LifecycleObserver {

    private var windowManager: WindowManager? = null
    private var overlayView: View? = null
    private var windowParams: WindowManager.LayoutParams? = null
    private var initialX = 0f
    private var initialY = 0f
    private var initialTouchX = 0f
    private var initialTouchY = 0f

    private var onOverlayActionListener: OnOverlayActionListener? = null

    interface OnOverlayActionListener {
        fun onMenuToggle(expanded: Boolean)
        fun onPageChanged(page: Int)
        fun onCheckBoxChanged(name: String, checked: Boolean)
        fun onSeekBarChanged(name: String, progress: Int)
        fun onSeekBarStopTracking(name: String, progress: Int)
        fun onRadioButtonChanged(group: String, name: String)
    }

    fun setOnOverlayActionListener(listener: OnOverlayActionListener) {
        onOverlayActionListener = listener
    }

    override fun onCreate() {
        super.onCreate()
        createOverlay()
    }

    private fun createOverlay() {
        windowManager = getSystemService(Context.WINDOW_SERVICE) as WindowManager

        windowParams = WindowManager.LayoutParams(
            WindowManager.LayoutParams.WRAP_CONTENT,
            WindowManager.LayoutParams.WRAP_CONTENT,
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY
            } else {
                WindowManager.LayoutParams.TYPE_SYSTEM_ALERT
            },
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE or
                WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL or
                WindowManager.LayoutParams.FLAG_WATCH_OUTSIDE_TOUCH,
            PixelFormat.TRANSLUCENT
        ).apply {
            gravity = Gravity.LEFT or Gravity.TOP
            x = 0
            y = 0
        }

        overlayView = OverlayView(this).apply {
            setOnTouchListener { v, event ->
                handleTouch(event)
                true
            }
        }

        windowManager?.addView(overlayView, windowParams)
    }

    private fun handleTouch(event: MotionEvent): Boolean {
        when (event.action) {
            MotionEvent.ACTION_DOWN -> {
                initialX = windowParams?.x?.toFloat() ?: 0f
                initialY = windowParams?.y?.toFloat() ?: 0f
                initialTouchX = event.rawX
                initialTouchY = event.rawY
                return true
            }
            MotionEvent.ACTION_MOVE -> {
                windowParams?.x = (initialX + (event.rawX - initialTouchX)).toInt()
                windowParams?.y = (initialY + (event.rawY - initialTouchY)).toInt()
                windowManager?.updateViewLayout(overlayView!!, windowParams!!)
                return true
            }
            MotionEvent.ACTION_UP -> {
                return true
            }
        }
        return false
    }

    fun showOverlay() {
        overlayView?.visibility = View.VISIBLE
    }

    fun hideOverlay() {
        overlayView?.visibility = View.GONE
    }

    fun isOverlayShowing(): Boolean {
        return overlayView?.visibility == View.VISIBLE
    }

    override fun onDestroy() {
        if (overlayView != null && windowManager != null) {
            windowManager?.removeView(overlayView)
            overlayView = null
        }
        super.onDestroy()
    }

    override fun onBind(intent: Intent?): IBinder? = null

    inner class OverlayView(context: Context) : FrameLayout(context) {
        init {
            // Inflate the cheat menu layout
            View.inflate(context, R.layout.overlay_cheat_menu, this)
            setupViews()
        }

        private fun setupViews() {
            // Setup all UI elements from the layout
            // This will be implemented with the actual layout
        }
    }
}