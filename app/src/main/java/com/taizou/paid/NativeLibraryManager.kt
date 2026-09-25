package com.taizou.paid

import android.content.Context
import android.util.Log
import java.io.File
import java.io.FileOutputStream
import java.io.InputStream

object NativeLibraryManager {
    private const val TAG = "NativeLibraryManager"

    private val nativeLibs = listOf(
        "charss", "charss2", "charss3", "espan", "exeChar", "fretzHAHA",
        "fuckmellee", "gay", "gayontop", "gayontopp", "haha", "heatExtractor",
        "ilove", "m13", "mc", "nosmoke", "pogiako", "pogiako2", "pogiako3",
        "sand", "sken", "skin", "snowb", "starzone", "thumbnail", "warden",
        "wow", "wow2", "xczSKINcpp", "kestrelbase", "angge", "banner", "rin",
        "CharssHAHA", "Cin", "Gundam", "HANDOG", "Kestrelsnow", "REISKIN2",
        "Rambo", "Roze", "TuwadNaForXielBabayaranKitaNangMahalDiMuraDMMeHAHAHAHAHNAMO",
        "fetas", "ferg"
    )

    private val arm64Libs = listOf(
        "char", "charss", "espan", "exeChar", "fretzHAHA", "gayontopp",
        "byy", "djobeibfuheihfihsubfubsuebi", "masarap", "new", "pogiako",
        "sandstorm", "sasakyanmotiteko", "skin", "skinn", "susubuinmonabatitenixiel",
        "wow", "xax", "xiel", "xielskins1", "ARNIXIELHAAHAHHATANGINAMO",
        "CharssHAHA", "Cin", "IDOLMOAKODAPATLANGWHAHAHAHAHAHAHHAMALAKIPATITEKOSAIYO",
        "LgmMissMoBaTiteNiXielNgangaKaNaHaHaHaAR", "MythicBembangKaKayXielHAHAHAHAHAHA",
        "PALAKIHANBURATXIELITOWHAHAHAHAH", "SRNIXIELAHHAHAAHSUBUINMOTITEKO",
        "TANGINAMOWHAHAHHAAHHAHASENDAKOTITEFORYOUMASASARAPANKAHWHAHAHAHAHAHAHAHATASTEMYREDDICKFUCKERWHAHAHHAHAHAHAHAHAHAHAHAHAHTANGINAMOOO",
        "TuwadNaForXielBabayaranKitaNangMahalDiMuraDMMeHAHAHAHAHNAMO",
        "XIELXIELXIELXIELL", "burat/ummmnakakainbaitoAhhhcameraWOHOIIPALDOPAPDOPLADOOO"
    )

    fun extractNativeLibraries(context: Context): Boolean {
        val filesDir = context.filesDir
        val libsDir = File(filesDir, "lib")
        val resDir = File(filesDir, "Res")

        libsDir.mkdirs()
        resDir.mkdirs()

        // Extract from assets/native to files/Res
        val assetManager = context.assets
        try {
            val resAssets = assetManager.list("native")
            resAssets?.forEach { fileName ->
                copyAsset(assetManager, "native/$fileName", File(resDir, fileName))
            }
        } catch (e: Exception) {
            Log.e(TAG, "Error listing native assets", e)
        }

        // Extract from jniLibs to files/lib (for arm64-v8a)
        val arm64Dir = File(libsDir, "arm64-v8a")
        arm64Dir.mkdirs()

        // The jniLibs are already packaged in the APK, but we need them in files dir for execution
        // We'll copy them from the APK's lib directory
        return true
    }

    private fun copyAsset(assetManager: android.content.res.AssetManager, assetPath: String, destFile: File) {
        var inputStream: InputStream? = null
        var outputStream: FileOutputStream? = null
        try {
            inputStream = assetManager.open(assetPath)
            outputStream = FileOutputStream(destFile)
            val buffer = ByteArray(8192)
            var length: Int
            while (inputStream.read(buffer).also { length = it } > 0) {
                outputStream.write(buffer, 0, length)
            }
            outputStream.flush()
            // Make executable
            destFile.setExecutable(true, false)
        } catch (e: Exception) {
            Log.e(TAG, "Failed to copy asset: $assetPath", e)
        } finally {
            inputStream?.close()
            outputStream?.close()
        }
    }

    fun getLibraryPath(context: Context, libName: String): String {
        val filesDir = context.filesDir
        val resDir = File(filesDir, "Res")
        val libFile = File(resDir, libName)
        if (libFile.exists()) {
            return libFile.absolutePath
        }
        // Fallback to arm64-v8a libs
        val arm64Dir = File(filesDir, "lib/arm64-v8a")
        val arm64File = File(arm64Dir, libName)
        if (arm64File.exists()) {
            return arm64File.absolutePath
        }
        return ""
    }

    fun executeBinary(context: Context, libName: String, args: String = "", useRoot: Boolean = false): Boolean {
        val path = getLibraryPath(context, libName)
        if (path.isEmpty()) {
            Log.e(TAG, "Library not found: $libName")
            return false
        }

        val cmd = if (useRoot) {
            "su -c 'chmod 777 $path && $path $args'"
        } else {
            "chmod 777 $path && $path $args"
        }

        try {
            val process = Runtime.getRuntime().exec(cmd)
            process.waitFor()
            return process.exitValue() == 0
        } catch (e: Exception) {
            Log.e(TAG, "Failed to execute binary: $libName", e)
            return false
        }
    }
}