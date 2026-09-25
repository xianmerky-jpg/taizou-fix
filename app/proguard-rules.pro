# Add project specific ProGuard rules here.
# You can control the set of applied configuration files using the
# proguardFiles setting in build.gradle.
#
# For more details, see
#   http://developer.android.com/guide/developing/tools/proguard.html

# Keep native library methods
-keep class com.taizou.paid.TaizouNative { *; }

# Keep application class
-keep class com.taizou.paid.TaizouApplication { *; }

# Keep MainActivity
-keep class com.taizou.paid.MainActivity { *; }

# Keep controller
-keep class com.taizou.paid.TaizouController { *; }

# Keep overlay service
-keep class com.taizou.paid.OverlayService { *; }

# Keep fragments and adapters
-keep class com.taizou.paid.PageAdapter { *; }
-keep class com.taizou.paid.PageFragment { *; }

# Keep Gson for config serialization
-keep class com.google.gson.** { *; }
-keepattributes Signature
-keepattributes *Annotation*
-keep class sun.misc.Unsafe { *; }

# Keep Kotlin coroutines
-keep class kotlinx.coroutines.** { *; }

# Keep AndroidX
-keep class androidx.** { *; }

# Keep Material Components
-keep class com.google.android.material.** { *; }