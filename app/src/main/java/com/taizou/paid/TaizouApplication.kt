package com.taizou.paid

import android.app.Application
import android.content.Context
import android.content.pm.PackageManager
import android.os.Build
import android.provider.Settings
import android.util.Log
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleObserver
import androidx.lifecycle.OnLifecycleEvent
import androidx.lifecycle.ProcessLifecycleOwner

class TaizouApplication : Application(), LifecycleObserver {

    override fun onCreate() {
        super.onCreate()
        // Persist uncaught crashes so the next launch can show what happened
        // instead of silently closing. Chained to the previous handler.
        val prevHandler = Thread.getDefaultUncaughtExceptionHandler()
        Thread.setDefaultUncaughtExceptionHandler { thread, e ->
            try {
                openFileOutput("crash.log", Context.MODE_PRIVATE).use {
                    it.write(Log.getStackTraceString(e).toByteArray())
                }
            } catch (ignored: Exception) {
                Log.e("TaizouApp", "Failed to write crash log", ignored)
            }
            if (prevHandler != null) {
                prevHandler.uncaughtException(thread, e)
            }
        }
        ProcessLifecycleOwner.get().lifecycle.addObserver(this)
        TaizouNative.initialize(this)
        // Unpack skin binaries + background video in background (25MB+ of
        // assets; never on the UI thread). Radio/skin features execute these
        // from filesDir, so without this step they silently do nothing.
        Thread {
            try {
                NativeLibraryManager.extractNativeLibraries(this@TaizouApplication)
            } catch (e: Exception) {
                Log.e("TaizouApp", "native asset extraction failed", e)
            }
        }.start()
    }

    @OnLifecycleEvent(Lifecycle.Event.ON_DESTROY)
    fun onAppDestroy() {
        TaizouNative.shutdown()
    }

    fun hasOverlayPermission(): Boolean {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            return Settings.canDrawOverlays(this)
        }
        return true
    }

    fun requestOverlayPermission() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && !hasOverlayPermission()) {
            val intent = android.content.Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION)
            intent.data = android.net.Uri.parse("package:$packageName")
            intent.addFlags(android.content.Intent.FLAG_ACTIVITY_NEW_TASK)
            startActivity(intent)
        }
    }
}