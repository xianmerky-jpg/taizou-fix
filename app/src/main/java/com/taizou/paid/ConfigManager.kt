package com.taizou.paid

import android.content.Context
import android.content.SharedPreferences
import com.google.gson.Gson
import com.google.gson.reflect.TypeToken

class ConfigManager(private val context: Context) {

    private val prefs: SharedPreferences = context.getSharedPreferences("taizou_config", Context.MODE_PRIVATE)
    private val gson = Gson()

    private val checkboxKeys = setOf(
        "report", "tut", "clogs", "floatmenu4", "memory", "quality",
        "wall", "redhack", "wo", "wo2", "amo", "fire", "norecoil", "nos",
        "noreload", "fscope", "fastsw", "speed", "advance", "crouch", "walk",
        "paldo", "noshakegun", "nop", "spect", "br", "un", "kinetic", "flashbang",
        "walkair", "nocrouch", "hit"
    )

    private val seekbarKeys = setOf(
        "aimbot_seekbar", "snowboard_seekbar", "diveb_seekbar", "br_seekbar", "mp_seekbar"
    )

    data class Config(
        val checkboxes: Map<String, Boolean> = emptyMap(),
        val seekbars: Map<String, Int> = emptyMap()
    )

    fun saveConfig(checkboxStates: Map<String, Boolean>, seekbarStates: Map<String, Int>) {
        val config = Config(
            checkboxes = checkboxStates.filterKeys { it in checkboxKeys },
            seekbars = seekbarStates.filterKeys { it in seekbarKeys }
        )
        val json = gson.toJson(config)
        prefs.edit().putString("config_json", json).apply()
    }

    fun loadConfig(): Config? {
        val json = prefs.getString("config_json", "") ?: return null
        if (json.isBlank()) return null
        return try {
            gson.fromJson(json, Config::class.java)
        } catch (e: Exception) {
            null
        }
    }

    fun getCheckboxState(key: String): Boolean {
        return prefs.getBoolean(key, false)
    }

    fun setCheckboxState(key: String, value: Boolean) {
        prefs.edit().putBoolean(key, value).apply()
    }

    fun getSeekbarProgress(key: String): Int {
        return prefs.getInt(key, 0)
    }

    fun setSeekbarProgress(key: String, value: Int) {
        prefs.edit().putInt(key, value).apply()
    }

    fun clearConfig() {
        prefs.edit().clear().apply()
    }
}