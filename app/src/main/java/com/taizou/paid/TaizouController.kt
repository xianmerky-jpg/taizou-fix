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
import android.widget.Toast
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

    @Suppress("UNUSED_PARAMETER")
    external fun applyAutoBypass(): Boolean

    @Suppress("UNUSED_PARAMETER")
    external fun isLibraryLoaded(pid: Int, libName: String): Boolean

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
        try {
            val text = queue.removeAt(0)
            tts?.speak(text, TextToSpeech.QUEUE_ADD, null, "taizou_tts")
        } finally {
            processing = false
        }
        if (queue.isNotEmpty()) processQueue()
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
    @Volatile
    private var bypassRunning = false

    @OnLifecycleEvent(Lifecycle.Event.ON_CREATE)
    fun onCreate() {
        // Native init runs popen(su)/file I/O that can block; keep it off the
        // UI thread. TTS queue works regardless (it buffers until ready).
        Thread {
            TaizouNative.init(context)
            loadConfig()
        }.start()
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
        // NOTE: context is the Activity, never the Application: check the
        // permission directly instead of casting to TaizouApplication
        // (that cast always yields null, which made START silently do nothing).
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && !Settings.canDrawOverlays(context)) {
            try {
                val intent = Intent(
                    Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                    Uri.parse("package:${context.packageName}")
                )
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                context.startActivity(intent)
            } catch (e: Exception) {
                Log.e("TaizouController", "Failed to open overlay settings", e)
            }
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

    /**
     * Ports the original AndLua startup flow: wait for CODM + libanogs.so
     * (15 x 2s), then after a 2s delay speak + write the 18 libanogs.so
     * bypass patches (+6s), then toast + speak success (+1s).
     * Runs once at a time on a background thread; safe to call per START tap.
     */
    fun startAutoBypass() {
        if (bypassRunning) return
        bypassRunning = true
        Thread {
            try {
                var ready = false
                var retries = 0
                while (retries <= 15 && !ready) {
                    val pid = TaizouNative.findProcessId("com.garena.game.codm")
                    if (pid > 0 && TaizouNative.isLibraryLoaded(pid, "libanogs.so")) {
                        ready = true
                    } else {
                        Thread.sleep(2000)
                        retries++
                    }
                }
                if (!ready) {
                    logToFile("CODM + libanogs.so not found after retries")
                } else {
                    logToFile("CODM + libanogs.so detected")
                    Thread.sleep(2000)
                    tts.speak("WAIT BYPASS INJECTED")
                    Thread.sleep(6000)
                    val ok = TaizouNative.applyAutoBypass()
                    logToFile(if (ok) "Bypassing Injected Successfully." else "Bypass patch failed")
                    Thread.sleep(1000)
                    (context as? Activity)?.runOnUiThread {
                        Toast.makeText(context, "ᴛᴀɪᴢᴏᴜ ʙʏᴘᴀss sᴜᴄᴄᴇs", Toast.LENGTH_SHORT).show()
                    }
                    tts.speak("Auto bypass succes")
                }
            } catch (e: InterruptedException) {
                // Bypass waiter stopped; not an error.
                Log.d("TaizouController", "auto-bypass interrupted", e)
            } finally {
                bypassRunning = false
            }
        }.start()
    }

    private fun logToFile(msg: String) {
        // App-private log: writing the original /storage/emulated/0 path is
        // blocked by scoped storage on modern Android.
        try {
            java.io.File(context.filesDir, "bypass_log.txt").appendText("[taguro] $msg\n")
        } catch (e: Exception) {
            Log.e("TaizouController", "logToFile failed", e)
        }
    }

    fun onLaunchClick() {
        listener.onLaunchClicked()
        launchCODM()
    }

    private fun launchCODM() {
        try {
            val pm = context.packageManager
            val intent = pm.getLaunchIntentForPackage("com.garena.game.codm")
            if (intent != null) {
                context.startActivity(intent)
            } else {
                Log.e("TaizouController", "CODM launch intent is null (not installed/invisible)")
                tts.speak("Call of Duty Mobile is not installed")
            }
        } catch (e: Exception) {
            Log.e("TaizouController", "Failed to launch CODM", e)
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

    /** Advisory only: patches still run (they no-op safely without game/root). */
    private fun checkPatchReady() {
        if (TaizouNative.findProcessId("com.garena.game.codm") <= 0) {
            Toast.makeText(context, "Launch CODM first", Toast.LENGTH_SHORT).show()
        } else if (!TaizouNative.isRootAvailable()) {
            Thread { TaizouNative.checkRoot() }.start()
            Toast.makeText(context, "Root access not detected", Toast.LENGTH_SHORT).show()
        }
    }

    fun onCheckBoxChanged(name: String, checked: Boolean) {
        checkPatchReady()
        TaizouNative.setCheckBoxState(name, checked)
        listener.onCheckBoxChanged(name, checked)
        speakFeature(name, checked)
    }

    fun onSeekBarChanged(name: String, progress: Int) {
        TaizouNative.setSeekBarProgress(name, progress)
        listener.onSeekBarChanged(name, progress)
    }

    fun onSeekBarStopTracking(name: String, progress: Int) {
        checkPatchReady()
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
            "clogs" to "Clear Logs",
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
        try {
            val intent = Intent(Intent.ACTION_VIEW, Uri.parse(url))
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            context.startActivity(intent)
        } catch (e: Exception) {
            Log.e("TaizouController", "No app to open $url", e)
        }
    }

    private fun showPriceDialog() {
        // Dialog handled in UI layer
    }
}