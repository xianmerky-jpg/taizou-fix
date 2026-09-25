package com.taizou.paid

import android.app.Activity
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Build
import android.provider.Settings
import android.speech.tts.TextToSpeech
import android.speech.tts.UtteranceProgressListener
import android.util.Log
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleObserver
import androidx.lifecycle.LifecycleOwner
import androidx.lifecycle.OnLifecycleEvent
import java.util.Locale
import java.util.concurrent.atomic.AtomicBoolean

object TaizouNative {
    private const val TAG = "TaizouNative"
    private var initialized = false
    @Volatile
    private var rootAvailable = false

    @Suppress("UNUSED_PARAMETER")
    external fun initialize(context: Context): Boolean

    @Suppress("UNUSED_PARAMETER")
    external fun shutdown()

    @Suppress("UNUSED_PARAMETER")
    external fun hasRootAccess(): Boolean

    @Suppress("UNUSED_PARAMETER")
    external fun findProcessId(packageName: String): Int

    @Suppress("UNUSED_PARAMETER")
    external fun applyMemoryPatch(libName: String, offset: Long, bytes: ByteArray): Boolean

    @Suppress("UNUSED_PARAMETER")
    external fun applyMemoryPatchOffset(libName: String, offset: Long, hexBytes: String): Boolean

    @Suppress("UNUSED_PARAMETER")
    external fun setCheckBoxState(name: String, checked: Boolean)

    @Suppress("UNUSED_PARAMETER")
    external fun setSeekBarProgress(name: String, progress: Int)

    @Suppress("UNUSED_PARAMETER")
    external fun setRadioButtonState(group: String, name: String)

    @Suppress("UNUSED_PARAMETER")
    external fun saveConfig(): String

    @Suppress("UNUSED_PARAMETER")
    external fun loadConfig(json: String): Boolean

    @Suppress("UNUSED_PARAMETER")
    external fun executeNativeBinary(binaryName: String, args: String)

    @Suppress("UNUSED_PARAMETER")
    external fun executeNativeBinaryRoot(binaryName: String, args: String)

    @Suppress("UNUSED_PARAMETER")
    external fun speakText(text: String)

    init {
        System.loadLibrary("taizou_core")
    }

    fun init(context: Context): Boolean {
        if (!initialized) {
            initialized = initialize(context)
            if (initialized) {
                rootAvailable = hasRootAccess()
                Log.d(TAG, "Native initialized, root: $rootAvailable")
            }
        }
        return initialized
    }

    fun isRootAvailable(): Boolean = rootAvailable

    fun checkRoot() {
        rootAvailable = hasRootAccess()
    }
}

class TaizouTTS(private val context: Context) : TextToSpeech.OnInitListener {
    private var tts: TextToSpeech? = null
    private val initialized = AtomicBoolean(false)
    private val queue = mutableListOf<String>()
    private var processing = false

    init {
        tts = TextToSpeech(context, this)
    }

    override fun onInit(status: Int) {
        if (status == TextToSpeech.SUCCESS) {
            tts?.setLanguage(Locale.getDefault())
            initialized.set(true)
            processQueue()
        } else {
            Log.e("TaizouTTS", "TTS initialization failed")
        }
    }

    fun speak(text: String) {
        if (text.isBlank()) return
        queue.add(text)
        processQueue()
    }

    private fun processQueue() {
        if (processing || queue.isEmpty() || !initialized.get()) return
        processing = true
        val text = queue.removeAt(0)
        tts?.speak(text, TextToSpeech.QUEUE_FLUSH, null, "taizou_tts")
    }

    fun shutdown() {
        tts?.shutdown()
        tts = null
    }
}

interface OnConfigChangeListener {
    fun onCheckBoxChanged(name: String, checked: Boolean)
    fun onSeekBarChanged(name: String, progress: Int)
    fun onRadioButtonChanged(group: String, name: String)
    fun onSaveConfig()
    fun onLoadConfig()
    fun onStartClicked()
    fun onStopClicked()
    fun onLaunchClicked()
    fun onOwnerClicked()
    fun onPriceClicked()
    fun onChannelClicked()
    fun onExitClicked()
}

class TaizouController(
    private val context: Context,
    private val listener: OnConfigChangeListener
) : LifecycleObserver {

    private val tts = TaizouTTS(context)
    private val configPrefs = context.getSharedPreferences("taizou_config", Context.MODE_PRIVATE)
    private var overlayShown = false

    @OnLifecycleEvent(Lifecycle.Event.ON_CREATE)
    fun onCreate() {
        TaizouNative.init(context)
        loadConfig()
        speakWelcome()
    }

    @OnLifecycleEvent(Lifecycle.Event.ON_DESTROY)
    fun onDestroy() {
        tts.shutdown()
    }

    private fun speakWelcome() {
        tts.speak("Welcome to Taizou CODM GR")
    }

    fun onStartClick() {
        // The su probe can block on a root-manager prompt; never run it on
        // the UI thread (would look like a freeze/crash on START).
        Thread { TaizouNative.checkRoot() }.start()
        if (!((context as? TaizouApplication)?.hasOverlayPermission() ?: false)) {
            (context as? TaizouApplication)?.requestOverlayPermission()
            tts.speak("Please allow display over other apps, then press back")
            return
        }
        overlayShown = true
        listener.onStartClicked()
    }

    fun onStopClick() {
        overlayShown = false
        listener.onStopClicked()
        // Exit the app
        (context as? Activity)?.finishAffinity()
        System.exit(0)
    }

    fun onLaunchClick() {
        listener.onLaunchClicked()
        launchCODM()
    }

    private fun launchCODM() {
        val pm = context.packageManager
        val intent = pm.getLaunchIntentForPackage("com.garena.game.codm")
        if (intent != null) {
            context.startActivity(intent)
        }
    }

    fun onOwnerClick() {
        listener.onOwnerClicked()
        openUrl("https://t.me/PrimeTaizou")
    }

    fun onPriceClick() {
        listener.onPriceClicked()
        showPriceDialog()
    }

    fun onChannelClick() {
        listener.onChannelClicked()
        openUrl("https://t.me/taizoumainchannel")
    }

    fun onExitClick() {
        listener.onExitClicked()
    }

    fun onCheckBoxChanged(name: String, checked: Boolean) {
        TaizouNative.setCheckBoxState(name, checked)
        listener.onCheckBoxChanged(name, checked)
        speakFeature(name, checked)
    }

    fun onSeekBarChanged(name: String, progress: Int) {
        TaizouNative.setSeekBarProgress(name, progress)
        listener.onSeekBarChanged(name, progress)
    }

    fun onSeekBarStopTracking(name: String, progress: Int) {
        TaizouNative.setSeekBarProgress(name, progress)
        val messages = mapOf(
            "aimbot_seekbar" to "AIMBOT ADJUSTED TO $progress percent",
            "snowboard_seekbar" to "SNOWBOARD TO $progress percent",
            "diveb_seekbar" to "DIVEB ADJUSTED TO $progress percent",
            "br_seekbar" to "IPAD VIEW ADJUSTED TO $progress percent",
            "mp_seekbar" to "MP VIEW $progress percent"
        )
        messages[name]?.let { tts.speak(it) }
    }

    fun onRadioButtonChanged(group: String, name: String) {
        TaizouNative.setRadioButtonState(group, name)
        listener.onRadioButtonChanged(group, name)
    }

    fun onSaveConfig() {
        val json = TaizouNative.saveConfig()
        configPrefs.edit().putString("config_json", json).apply()
        listener.onSaveConfig()
        tts.speak("Config saved")
    }

    fun onLoadConfig() {
        val json = configPrefs.getString("config_json", "") ?: ""
        if (json.isNotBlank()) {
            TaizouNative.loadConfig(json)
            listener.onLoadConfig()
            tts.speak("Config loaded")
        }
    }

    private fun loadConfig() {
        val json = configPrefs.getString("config_json", "") ?: ""
        if (json.isNotBlank()) {
            TaizouNative.loadConfig(json)
        }
    }

    private fun speakFeature(name: String, checked: Boolean) {
        val action = if (checked) "activated" else "deactivated"
        val featureNames = mapOf(
            "report" to "Hold Report",
            "tut" to "Skip Tutorial",
            "floatmenu4" to "Unlock FPS",
            "memory" to "Memory Stable",
            "quality" to "HD Effects",
            "wall" to "Wallhack",
            "redhack" to "Red Hack",
            "hit" to "Extended Hitbox",
            "fscope" to "Fast Scope",
            "fastsw" to "Fast Switch",
            "advance" to "Advance",
            "spect" to "No Spectator Delay",
            "wo" to "Outline",
            "nos" to "No Spread",
            "noreload" to "No Reload",
            "norecoil" to "No Recoil",
            "speed" to "Speed Walk",
            "wo2" to "Outline 2",
            "amo" to "Unlimited Ammo",
            "fire" to "Rapid Fire",
            "paldo" to "Long Execute",
            "noshakegun" to "No Shake",
            "br" to "BR Tags",
            "pump" to "Pump Boost",
            "nop" to "No Parachute",
            "walk" to "Walk Underwater",
            "crouch" to "No Crouch",
            "un" to "Blueprint"
        )
        featureNames[name]?.let { tts.speak("$it $action") }
    }

    private fun openUrl(url: String) {
        val intent = Intent(Intent.ACTION_VIEW, Uri.parse(url))
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        context.startActivity(intent)
    }

    private fun showPriceDialog() {
        // Dialog handled in UI layer
    }
}