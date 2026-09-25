package com.taizou.paid

import android.app.AlertDialog
import android.content.Context
import android.content.Intent
import android.content.pm.ActivityInfo
import android.content.res.Configuration
import android.graphics.BitmapFactory
import android.graphics.Color
import android.graphics.PorterDuff
import android.graphics.Typeface
import android.graphics.drawable.ColorDrawable
import android.graphics.drawable.GradientDrawable
import android.media.MediaPlayer
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.provider.Settings
import android.speech.tts.TextToSpeech
import android.util.DisplayMetrics
import android.util.Log
import android.view.Gravity
import android.view.MotionEvent
import android.view.View
import android.view.ViewGroup
import android.view.WindowManager
import android.view.animation.TranslateAnimation
import android.widget.Button
import android.widget.CheckBox
import android.widget.CompoundButton
import android.widget.FrameLayout
import android.widget.ImageView
import android.widget.LinearLayout
import android.widget.RadioButton
import android.widget.RadioGroup
import android.widget.ScrollView
import android.widget.SeekBar
import android.widget.Switch
import android.widget.TextClock
import android.widget.TextView
import android.widget.Toast
import android.widget.ToggleButton
import android.widget.VideoView
import androidx.appcompat.app.AppCompatActivity
import androidx.cardview.widget.CardView
import androidx.core.content.ContextCompat
import androidx.fragment.app.DialogFragment
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleObserver
import androidx.lifecycle.OnLifecycleEvent
import androidx.viewpager.widget.ViewPager
import com.google.gson.Gson
import com.taizou.paid.databinding.ActivityMainBinding
import com.taizou.paid.databinding.OverlayCheatMenuBinding
import com.taizou.paid.PriceDialogFragment
import java.io.File
import java.util.Locale
import java.util.concurrent.atomic.AtomicBoolean

class MainActivity : AppCompatActivity(), TaizouController.OnConfigChangeListener, LifecycleObserver {

    private lateinit var binding: ActivityMainBinding
    private lateinit var controller: TaizouController
    private lateinit var tts: TextToSpeech
    private var ttsInitialized = false

    // Overlay views
    private var overlayWindowManager: WindowManager? = null
    private var overlayView: View? = null
    private var overlayParams: WindowManager.LayoutParams? = null
    private var overlayShown = false
    private var cheatMenuExpanded = false

    // Touch handling for overlay
    private var initialTouchX = 0f
    private var initialTouchY = 0f
    private var initialWindowX = 0f
    private var initialWindowY = 0f

    // Page navigation
    private var currentPage = 0

    // UI References for overlay
    private var overlayBinding: OverlayCheatMenuBinding? = null
    private var pageAdapter: PageAdapter? = null
    private var pg: ViewPager? = null

    // Menu buttons
    private var menu1: ImageView? = null
    private var menu2: ImageView? = null
    private var menu3: ImageView? = null
    private var menu4: ImageView? = null
    private var menu6: ImageView? = null

    // Clock animation
    private val clockHandler = Handler(Looper.getMainLooper())
    private var clockRunnable: Runnable? = null

    // ECG animation
    private var ecgTimer: Thread? = null
    private var ecgRunning = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Fullscreen setup
        window.addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN)
        window.clearFlags(WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN)
        requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        // Hide action bar
        supportActionBar?.hide()

        // Status bar color
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            window.addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS)
            window.statusBarColor = Color.parseColor("#FFA50000")
        }

        controller = TaizouController(this, this)
        lifecycle.addObserver(this)
        lifecycle.addObserver(controller)

        // Initialize TTS
        initTTS()

        // Setup video background
        setupVideo()

        // Setup main UI
        setupMainUI()

        // Setup overlay (floating cheat menu)
        setupOverlay()

        // Welcome toast and TTS
        showCustomToast("Welcome to Taizou CODM GR")
        speakText("Welcome to Taizou CODM GR")

        // Anti-debug checks
        performAntiDebugChecks()
    }

    private fun initTTS() {
        tts = TextToSpeech(this, object : TextToSpeech.OnInitListener {
            override fun onInit(status: Int) {
                if (status == TextToSpeech.SUCCESS) {
                    ttsInitialized = true
                    tts.setLanguage(Locale.getDefault())
                } else {
                    Log.e("TTS", "Initialization failed")
                }
            }
        })
    }

    private fun speakText(text: String) {
        if (ttsInitialized && !text.isBlank()) {
            tts.speak(text, TextToSpeech.QUEUE_FLUSH, null, "taizou_tts")
        }
    }

    private fun setupVideo() {
        val videoPath = File(filesDir, "bg.mp4").absolutePath
        if (File(videoPath).exists()) {
            binding.video.setVideoPath(videoPath)
            binding.video.setOnPreparedListener { mp ->
                mp.isLooping = true
                mp.start()
                binding.video.setBackgroundColor(Color.TRANSPARENT)
            }
            binding.video.start()
        }
    }

    private fun setupMainUI() {
        // Style buttons
        styleButton(binding.cardStart, binding.start, 0xFF6E6E6E, 0xFF1A1A1A)
        styleButton(binding.cardStop, binding.stop, 0xFF6E6E6E, 0xFF1A1A1A)
        styleButton(binding.cardLaunch, binding.game, 0xFF6E6E6E, 0xFF1A1A1A)

        binding.start.setOnCheckedChangeListener { _, checked ->
            if (checked) {
                waterDropAnimation(binding.game, 150)
                controller.onStartClick()
            } else {
                controller.onStopClick()
            }
        }

        binding.stop.setOnClickListener {
            waterDropAnimation(binding.game, 150)
            controller.onStopClick()
        }

        binding.game.setOnClickListener {
            waterDropAnimation(it, 150)
            controller.onLaunchClick()
        }

        binding.tg1.setOnClickListener {
            waterDropAnimation(it, 80)
            controller.onOwnerClick()
        }

        binding.tg2.setOnClickListener {
            waterDropAnimation(it, 80)
            controller.onPriceClick()
            PriceDialogFragment().show(supportFragmentManager, "PriceDialog")
        }

        binding.tg3.setOnClickListener {
            waterDropAnimation(it, 80)
            controller.onChannelClick()
        }
    }

    private fun styleButton(card: CardView, button: View, outlineColor: Int, fillColor: Int) {
        val drawable = GradientDrawable().apply {
            shape = GradientDrawable.RECTANGLE
            cornerRadius = 20f
            setColor(fillColor)
            setStroke(2, outlineColor)
        }
        button.background = drawable
    }

    private fun waterDropAnimation(view: View, duration: Int) {
        view.animate()
            .scaleX(0.8f).scaleY(0.8f)
            .setDuration(duration / 3)
            .withEndAction {
                view.animate()
                    .scaleX(1.3f).scaleY(1.3f)
                    .setDuration(duration / 3)
                    .withEndAction {
                        view.animate()
                            .scaleX(0.9f).scaleY(0.9f)
                            .setDuration(duration / 3)
                            .withEndAction {
                                view.animate()
                                    .scaleX(1f).scaleY(1f)
                                    .setDuration(duration / 3)
                            }
                            .start()
                    }
                    .start()
            }
            .start()
    }

    private fun setupOverlay() {
        overlayWindowManager = getSystemService(Context.WINDOW_SERVICE) as WindowManager

        overlayParams = WindowManager.LayoutParams(
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
            android.graphics.PixelFormat.TRANSLUCENT
        ).apply {
            gravity = Gravity.LEFT or Gravity.TOP
            x = 0
            y = 0
        }

        overlayBinding = OverlayCheatMenuBinding.inflate(layoutInflater)
        overlayView = overlayBinding?.root

        overlayView?.setOnTouchListener { _, event ->
            handleOverlayTouch(event)
            true
        }

        setupOverlayViews()
    }

    private fun setupOverlayViews() {
        overlayBinding?.apply {
            // Setup menu buttons
            menu1 = root.findViewById(R.id.menu1)
            menu2 = root.findViewById(R.id.menu2)
            menu3 = root.findViewById(R.id.menu3)
            menu4 = root.findViewById(R.id.menu4)
            menu6 = root.findViewById(R.id.menu6)

            pg = root.findViewById(R.id.pg)
            pageAdapter = PageAdapter(this@MainActivity)
            pg?.adapter = pageAdapter
            pg?.offscreenPageLimit = 5

            // Setup clock animation
            setupClockAnimation()

            // Setup ECG animation
            setupECGAnimation()

            // Menu click listeners
            menu1?.setOnClickListener { onMenuClick(0) }
            menu2?.setOnClickListener { onMenuClick(1) }
            menu3?.setOnClickListener { onMenuClick(2) }
            menu4?.setOnClickListener { onMenuClick(3) }
            menu6?.setOnClickListener { onMenuClick(5) }

            pg?.addOnPageChangeListener(object : ViewPager.OnPageChangeListener {
                override fun onPageScrolled(position: Int, positionOffset: Float, positionOffsetPixels: Int) {}
                override fun onPageSelected(position: Int) {
                    currentPage = position
                    updateMenuButtonStyles()
                }
                override fun onPageScrollStateChanged(state: Int) {}
            })

            // Header toggle
            img1.setOnClickListener { toggleCheatMenu() }
            hideBtn.setOnClickListener { hideOverlay() }
            floatingEyeIcon.setOnClickListener { showOverlay() }

            win_move.setOnTouchListener { _, event -> handleOverlayTouch(event) }
            menu?.setOnTouchListener { _, event -> handleOverlayTouch(event) }

            // Style menu buttons
            styleCircleButton(menu!!, 0xFF000000, 20, 0xFF000000)
            styleCircleButton3(cheatMenu!!, 0xC13A3A3A, 20, 0xFF00FFFF)
            styleCircleButton(menu1!!, 0xC13A3A3A, 20, 0xFF00FFFF)

            // Checkbox drawable tint
            report?.buttonDrawable?.setColorFilter(
                PorterDuffColorFilter(0xFFFFC600, PorterDuff.Mode.SRC_ATOP)
            )

            // Initialize checkboxes from config
            initializeCheckboxes()
            initializeSeekBars()
            initializeRadioButtons()
        }
    }

    private fun handleOverlayTouch(event: MotionEvent): Boolean {
        when (event.action) {
            MotionEvent.ACTION_DOWN -> {
                initialWindowX = overlayParams?.x?.toFloat() ?: 0f
                initialWindowY = overlayParams?.y?.toFloat() ?: 0f
                initialTouchX = event.rawX
                initialTouchY = event.rawY
                true
            }
            MotionEvent.ACTION_MOVE -> {
                overlayParams?.x = (initialWindowX + (event.rawX - initialTouchX)).toInt()
                overlayParams?.y = (initialWindowY + (event.rawY - initialTouchY)).toInt()
                overlayWindowManager?.updateViewLayout(overlayView!!, overlayParams!!)
                true
            }
            MotionEvent.ACTION_UP -> true
            else -> false
        }
    }

    private fun toggleCheatMenu() {
        cheatMenuExpanded = !cheatMenuExpanded
        overlayBinding?.cheatMenu?.visibility = if (cheatMenuExpanded) View.VISIBLE else View.GONE

        val iconRes = if (cheatMenuExpanded) R.drawable.ic_to_bottom else R.drawable.ic_to_top
        overlayBinding?.img1?.setImageResource(iconRes)
    }

    private fun showOverlay() {
        if (overlayView != null && overlayParams != null && !overlayShown) {
            if (Settings.canDrawOverlays(this)) {
                overlayWindowManager?.addView(overlayView, overlayParams)
                overlayShown = true
                overlayBinding?.floatingEyeIcon?.visibility = View.GONE
                overlayBinding?.menu?.visibility = View.VISIBLE
                showCustomToast("IMGUI Restored")
                speakText("IMGUI Restored")
            } else {
                requestOverlayPermission()
            }
        }
    }

    private fun hideOverlay() {
        if (overlayShown) {
            overlayWindowManager?.removeView(overlayView)
            overlayShown = false
            overlayBinding?.floatingEyeIcon?.visibility = View.VISIBLE
            showCustomToast("IMGUI Hidden")
            speakText("IMGUI Hidden")
        }
    }

    private fun onMenuClick(page: Int) {
        pg?.currentItem = page
        updateMenuButtonStyles()
    }

    private fun updateMenuButtonStyles() {
        val menus = listOf(menu1, menu2, menu3, menu4, menu6)
        menus.forEachIndexed { index, menu ->
            menu?.let {
                val isSelected = index == currentPage
                styleCircleButton2(it, 0x00000000, 20, if (isSelected) 0xFF00FFFF else 0xFF00FFFF)
            }
        }
    }

    private fun setupClockAnimation() {
        val clock = overlayBinding?.clock
        clockRunnable = Runnable { drawAnalogClock(clock) }
        clockHandler.post(clockRunnable!!)
    }

    private fun drawAnalogClock(clock: ImageView?) {
        clock?.let { iv ->
            val w = iv.width
            val h = iv.height
            if (w == 0 || h == 0) return

            val bitmap = android.graphics.Bitmap.createBitmap(w, h, android.graphics.Bitmap.Config.ARGB_8888)
            val canvas = android.graphics.Canvas(bitmap)
            val paint = android.graphics.Paint().apply { isAntiAlias = true }

            val cx = w / 2f
            val cy = h / 2f
            val maxRadius = min(w, h) / 2f
            val radius = maxRadius * 0.85f

            val GREEN = 0xFF00FF00
            val RED = 0xFFFF0000
            val WHITE = 0xFFFFFFFF

            // Outer glow
            paint.style = android.graphics.Paint.Style.STROKE
            paint.strokeWidth = max(2f, maxRadius * 0.05f)
            paint.color = GREEN
            paint.setShadowLayer(maxRadius * 0.15f, 0f, 0f, GREEN)
            canvas.drawCircle(cx, cy, radius, paint)
            paint.clearShadowLayer()

            // Inner circle
            paint.strokeWidth = max(1f, maxRadius * 0.02f)
            paint.color = 0xFF193D29
            canvas.drawCircle(cx, cy, radius * 0.9f, paint)

            // Hour marks
            paint.color = GREEN
            for (i in 0..59) {
                val angle = Math.toRadians(i * 6.0)
                val outer = radius * 0.88
                val inner = if (i % 5 == 0) {
                    paint.strokeWidth = max(2f, maxRadius * 0.04f)
                    radius * 0.70
                } else {
                    paint.strokeWidth = max(1f, maxRadius * 0.015f)
                    radius * 0.80
                }
                val x1 = cx + Math.sin(angle) * inner
                val y1 = cy - Math.cos(angle) * inner
                val x2 = cx + Math.sin(angle) * outer
                val y2 = cy - Math.cos(angle) * outer
                canvas.drawLine(x1, y1, x2, y2, paint)
            }

            // Numbers
            paint.style = android.graphics.Paint.Style.FILL
            paint.color = WHITE
            paint.textSize = radius * 0.35
            paint.typeface = Typeface.DEFAULT_BOLD
            paint.textAlign = android.graphics.Paint.Align.CENTER
            val nums = mapOf(12 to "12", 3 to "3", 6 to "6", 9 to "9")
            for ((n, text) in nums) {
                val angle = Math.toRadians(n * 30.0)
                val x = cx + Math.sin(angle) * (radius * 0.45)
                val y = cy - Math.cos(angle) * (radius * 0.45)
                canvas.drawText(text, x, y + paint.textSize / 3, paint)
            }

            // Current time
            val now = java.util.Calendar.getInstance()
            val hour = now.get(java.util.Calendar.HOUR) % 12
            val minute = now.get(java.util.Calendar.MINUTE)
            val second = now.get(java.util.Calendar.SECOND)

            val hourAngle = Math.toRadians((hour * 30) + (minute * 0.5))
            val minuteAngle = Math.toRadians((minute * 6) + (second * 0.1))
            val secondAngle = Math.toRadians(second * 6)

            // Hour hand
            paint.color = WHITE
            paint.strokeWidth = max(3f, maxRadius * 0.08f)
            paint.strokeCap = android.graphics.Paint.Cap.ROUND
            var hx = cx + Math.sin(hourAngle) * (radius * 0.48)
            var hy = cy - Math.cos(hourAngle) * (radius * 0.48)
            canvas.drawLine(cx, cy, hx, hy, paint)

            // Minute hand
            paint.color = GREEN
            paint.strokeWidth = max(2f, maxRadius * 0.05f)
            var mx = cx + Math.sin(minuteAngle) * (radius * 0.68)
            var my = cy - Math.cos(minuteAngle) * (radius * 0.68)
            canvas.drawLine(cx, cy, mx, my, paint)

            // Second hand
            paint.color = RED
            paint.strokeWidth = max(1f, maxRadius * 0.02f)
            var sx = cx + Math.sin(secondAngle) * (radius * 0.78)
            var sy = cy - Math.cos(secondAngle) * (radius * 0.78)
            canvas.drawLine(cx, cy, sx, sy, paint)

            // Center dots
            paint.color = WHITE
            canvas.drawCircle(cx, cy, max(2f, maxRadius * 0.06f), paint)
            paint.color = RED
            canvas.drawCircle(cx, cy, max(1f, maxRadius * 0.03f), paint)

            iv.setImageBitmap(bitmap)
        }

        clockHandler.postDelayed(clockRunnable!!, 1000)
    }

    private fun setupECGAnimation() {
        ecgRunning = true
        ecgTimer = Thread {
            var step = 0
            val points = mutableListOf<Float>()
            val maxPoints = 50
            val handler = Handler(Looper.getMainLooper())

            while (ecgRunning) {
                Thread.sleep(12)
                handler.post {
                    val ecgView = overlayBinding?.ecg_view
                    ecgView?.let { view ->
                        val w = view.width
                        val h = view.height
                        if (h <= 0) return@post

                        val centerY = h / 2f
                        step++
                        var nextY: Float

                        val m = step % 30
                        when (m) {
                            5 -> nextY = centerY - 10
                            10 -> nextY = centerY + 10
                            11 -> nextY = centerY - 90
                            12 -> nextY = centerY + 40
                            15 -> nextY = centerY - 15
                            else -> nextY = centerY + (Math.random() * 40 - 20).toFloat() / 10
                        }

                        points.add(nextY)
                        if (points.size > maxPoints) points.removeAt(0)

                        drawECG(view, points, w, h, centerY)
                    }
                }
            }
        }.apply { start() }
    }

    private fun drawECG(view: View, points: List<Float>, w: Int, h: Int, centerY: Float) {
        val bitmap = android.graphics.Bitmap.createBitmap(w, h, android.graphics.Bitmap.Config.ARGB_8888)
        val canvas = android.graphics.Canvas(bitmap)
        val paint = android.graphics.Paint().apply {
            isAntiAlias = true
            style = android.graphics.Paint.Style.STROKE
            strokeWidth = 6f
            color = 0xFF00FF00
            strokeCap = android.graphics.Paint.Cap.ROUND
            strokeJoin = android.graphics.Paint.Join.ROUND
            setShadowLayer(25f, 0f, 0f, 0xFF00FF00)
        }

        // Grid
        val gridPaint = android.graphics.Paint().apply {
            color = 0x3300FF00
            style = android.graphics.Paint.Style.STROKE
            strokeWidth = 1.5f
            isAntiAlias = true
        }
        val stepX = w / 15f
        for (i in 0..15) canvas.drawLine(i * stepX, 0f, i * stepX, h.toFloat(), gridPaint)
        val stepY = h / 6f
        for (i in 0..6) canvas.drawLine(0f, i * stepY, w.toFloat(), i * stepY, gridPaint)

        // Wave
        if (points.size > 2) {
            for (i in 2 until points.size) {
                val alpha = ((i.toFloat() / points.size) * 255).toInt()
                paint.alpha = alpha

                val x1 = (w / (maxPoints - 1).toFloat()) * (i - 2)
                val y1 = points[i - 1]
                val x2 = (w / (maxPoints - 1).toFloat()) * (i - 1)
                val y2 = points[i]

                canvas.drawLine(x1, y1, x2, y2, paint)

                if (i == points.size - 1) {
                    val scanPaint = android.graphics.Paint().apply {
                        color = 0xAA00FF00
                        strokeWidth = 4f
                        setShadowLayer(30f, 0f, 0f, 0xFF00FF00)
                    }
                    canvas.drawLine(x2, 0f, x2, h.toFloat(), scanPaint)
                }
            }
        }

        view.setBackground(android.graphics.drawable.BitmapDrawable(resources, bitmap))
    }

    private fun initializeCheckboxes() {
        val checkboxes = listOf(
            overlayBinding?.report, overlayBinding?.tut, overlayBinding?.clogs,
            overlayBinding?.floatmenu4, overlayBinding?.memory, overlayBinding?.quality,
            overlayBinding?.wall, overlayBinding?.redhack, overlayBinding?.wo, overlayBinding?.wo2,
            overlayBinding?.amo, overlayBinding?.fire, overlayBinding?.norecoil, overlayBinding?.nos,
            overlayBinding?.noreload, overlayBinding?.fscope, overlayBinding?.fastsw,
            overlayBinding?.speed, overlayBinding?.advance, overlayBinding?.crouch, overlayBinding?.walk,
            overlayBinding?.paldo, overlayBinding?.noshakegun, overlayBinding?.nop, overlayBinding?.spect,
            overlayBinding?.br, overlayBinding?.un
        )

        checkboxes.forEach { cb ->
            cb?.setOnCheckedChangeListener { _, checked ->
                val name = getCheckboxName(cb!!)
                if (!name.isNullOrBlank()) {
                    controller.onCheckBoxChanged(name, checked)
                }
            }
        }
    }

    private fun getCheckboxName(cb: CheckBox): String? {
        return when (cb.id) {
            R.id.report -> "report"
            R.id.tut -> "tut"
            R.id.clogs -> "clogs"
            R.id.floatmenu4 -> "floatmenu4"
            R.id.memory -> "memory"
            R.id.quality -> "quality"
            R.id.wall -> "wall"
            R.id.redhack -> "redhack"
            R.id.wo -> "wo"
            R.id.wo2 -> "wo2"
            R.id.amo -> "amo"
            R.id.fire -> "fire"
            R.id.norecoil -> "norecoil"
            R.id.nos -> "nos"
            R.id.noreload -> "noreload"
            R.id.fscope -> "fscope"
            R.id.fastsw -> "fastsw"
            R.id.speed -> "speed"
            R.id.advance -> "advance"
            R.id.crouch -> "crouch"
            R.id.walk -> "walk"
            R.id.paldo -> "paldo"
            R.id.noshakegun -> "noshakegun"
            R.id.nop -> "nop"
            R.id.spect -> "spect"
            R.id.br -> "br"
            R.id.un -> "un"
            else -> null
        }
    }

    private fun initializeSeekBars() {
        val seekBars = mapOf(
            overlayBinding?.aimbot_seekbar to "aimbot_seekbar",
            overlayBinding?.snowboard_seekbar to "snowboard_seekbar",
            overlayBinding?.diveb_seekbar to "diveb_seekbar",
            overlayBinding?.br_seekbar to "br_seekbar",
            overlayBinding?.mp_seekbar to "mp_seekbar"
        )

        seekBars.forEach { (sb, name) ->
            sb?.apply {
                val textView = when (name) {
                    "aimbot_seekbar" -> overlayBinding?.aimbot_text
                    "snowboard_seekbar" -> overlayBinding?.snowboard_text
                    "diveb_seekbar" -> overlayBinding?.diveb_text
                    "br_seekbar" -> overlayBinding?.br_text
                    "mp_seekbar" -> overlayBinding?.mp_text
                    else -> null
                }

                setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
                    override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                        textView?.text = "${getSeekBarLabel(name)} ($progress%)"
                    }

                    override fun onStartTrackingTouch(seekBar: SeekBar?) {}

                    override fun onStopTrackingTouch(seekBar: SeekBar?) {
                        controller.onSeekBarStopTracking(name, progress)
                    }
                })
            }
        }
    }

    private fun getSeekBarLabel(name: String): String {
        return when (name) {
            "aimbot_seekbar" -> "AIMBOT"
            "snowboard_seekbar" -> "SNOWBOARD"
            "diveb_seekbar" -> "DIVE BOOST"
            "br_seekbar" -> "IPAD VIEW"
            "mp_seekbar" -> "MP VIEW"
            else -> name.uppercase()
        }
    }

    private fun initializeRadioButtons() {
        // Setup radio groups from overlay pages
        // This would be done when each page fragment is created
    }

    private fun showCustomToast(message: String) {
        val inflater = layoutInflater
        val layout = inflater.inflate(R.layout.custom_toast, findViewById(R.id.toast_root))
        val text = layout.findViewById<TextView>(R.id.toast_text)
        text.text = message

        val toast = Toast(applicationContext)
        toast.duration = Toast.LENGTH_SHORT
        toast.view = layout
        toast.gravity = Gravity.BOTTOM, 0, 120
        toast.show()
    }

    private fun performAntiDebugChecks() {
        val suspiciousPackages = listOf(
            "com.guoshi.httpcanary",
            "sstool.only.com.sstool",
            "sstool.serdadu",
            "cn.lovesong.luadec",
            "com.n0n3m4.droidc"
        )

        suspiciousPackages.forEach { pkg ->
            try {
                packageManager.getPackageInfo(pkg, 0)
                showCustomToast("Error: Cannot attach to mainCode")
                finishAffinity()
                System.exit(0)
            } catch (e: Exception) {
                // Package not found, continue
            }
        }
    }

    private fun styleCircleButton(view: View, insideColor: Int, radius: Int, strokeColor: Int) {
        val drawable = GradientDrawable().apply {
            shape = GradientDrawable.RECTANGLE
            cornerRadii = floatArrayOf(radius.toFloat(), radius.toFloat(), radius.toFloat(), radius.toFloat(),
                radius.toFloat(), radius.toFloat(), radius.toFloat(), radius.toFloat())
            setColor(insideColor)
            setStroke(2, strokeColor)
        }
        view.background = drawable
    }

    private fun styleCircleButton2(view: View, insideColor: Int, radius: Int, strokeColor: Int) {
        val drawable = GradientDrawable().apply {
            shape = GradientDrawable.RECTANGLE
            cornerRadii = floatArrayOf(15f, 15f, 15f, 15f, 15f, 15f, 15f, 15f)
            setColor(insideColor)
            setStroke(2, strokeColor)
        }
        view.background = drawable
    }

    private fun styleCircleButton3(view: View, insideColor: Int, radius: Int, strokeColor: Int) {
        val drawable = GradientDrawable().apply {
            shape = GradientDrawable.RECTANGLE
            cornerRadii = floatArrayOf(radius.toFloat(), radius.toFloat(), radius.toFloat(), radius.toFloat(),
                radius.toFloat(), radius.toFloat(), radius.toFloat(), radius.toFloat())
            setColor(insideColor)
            setStroke(5, strokeColor)
        }
        view.background = drawable
    }

    private fun makeOvalButton(btn: Button, bgColor: Int, strokeColor: Int) {
        val drawable = GradientDrawable().apply {
            shape = GradientDrawable.RECTANGLE
            cornerRadius = 50f
            setColor(bgColor)
            setStroke(3, strokeColor)
        }
        btn.background = drawable
    }

    override fun onCheckBoxChanged(name: String, checked: Boolean) {
        // Update UI if needed
    }

    override fun onSeekBarChanged(name: String, progress: Int) {
        // Update UI text
    }

    override fun onRadioButtonChanged(group: String, name: String) {
        // Handle radio button changes
    }

    override fun onSaveConfig() {
        showCustomToast("Config Saved!")
    }

    override fun onLoadConfig() {
        showCustomToast("Config Loaded!")
    }

    override fun onStartClicked() {
        showOverlay()
    }

    override fun onStopClicked() {
        hideOverlay()
        finishAffinity()
        System.exit(0)
    }

    override fun onLaunchClicked() {
        // Handled in controller
    }

    override fun onOwnerClicked() {}

    override fun onPriceClicked() {}

    override fun onChannelClicked() {}

    override fun onExitClicked() {
        val builder = AlertDialog.Builder(this)
        builder.setTitle("REMINDER BEFORE EXITING")
        builder.setCancelable(false)
        builder.setMessage("ARE YOU SURE YOU WANT TO EXIT IN TAIZOU INJECTOR?? DON'T FORGET TO FEEDBACK THANKS YOU")
        builder.setPositiveButton("EXIT") { _, _ ->
            finishAffinity()
            System.exit(0)
        }
        builder.setNegativeButton("NO", null)
        builder.setNeutralButton("FEEDBACK TO OWNER") { _, _ ->
            val intent = Intent(Intent.ACTION_VIEW, Uri.parse("https://t.me/PrimeTaizou"))
            startActivity(intent)
            finishAffinity()
            System.exit(0)
        }
        val dialog = builder.create()
        dialog.show()
        dialog.window?.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))
    }

    @OnLifecycleEvent(Lifecycle.Event.ON_DESTROY)
    fun onAppDestroy() {
        ecgRunning = false
        ecgTimer?.interrupt()
        clockHandler.removeCallbacksAndMessages(null)
        tts.shutdown()
        if (overlayShown) {
            hideOverlay()
        }
        TaizouNative.shutdown()
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == 1001 && grantResults.isNotEmpty() && grantResults[0] == android.content.pm.PackageManager.PERMISSION_GRANTED) {
            showOverlay()
        }
    }

    private fun requestOverlayPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && !Settings.canDrawOverlays(this)) {
            val intent = Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION)
            intent.data = Uri.parse("package:$packageName")
            startActivityForResult(intent, 1001)
        }
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode == 1001) {
            if (Settings.canDrawOverlays(this)) {
                showOverlay()
            }
        }
    }

    override fun onConfigurationChanged(newConfig: Configuration) {
        super.onConfigurationChanged(newConfig)
        if (newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE) {
            // Already in landscape
        }
    }
}