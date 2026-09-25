package com.taizou.paid

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.FragmentActivity
import androidx.viewpager.widget.PagerAdapter

// NOTE: plain view-based PagerAdapter, deliberately NOT FragmentPagerAdapter.
// The ViewPager lives in a WindowManager overlay window, outside the activity
// content view, so the activity's FragmentManager can never resolve its
// container id and crashes with "No view found for id pg for fragment
// PageFragment". Fragments are unnecessary here anyway: each page is just an
// inflated layout whose views are wired via onPageInflated().
class PageAdapter(private val activity: FragmentActivity) : PagerAdapter() {

    private val pageLayouts = intArrayOf(
        R.layout.page_main,
        R.layout.page_memory,
        R.layout.page_adjustable,
        R.layout.page_skins,
        R.layout.page_legendary_skins,
        R.layout.page_settings
    )

    override fun getCount(): Int = pageLayouts.size

    override fun isViewFromObject(view: View, obj: Any): Boolean = view === obj

    override fun instantiateItem(container: ViewGroup, position: Int): Any {
        val view = activity.layoutInflater.inflate(pageLayouts[position], container, false)
        container.addView(view)
        (activity as? MainActivity)?.onPageInflated(view)
        return view
    }

    override fun destroyItem(container: ViewGroup, position: Int, obj: Any) {
        container.removeView(obj as View)
    }

    override fun getPageTitle(position: Int): CharSequence? {
        return when (position) {
            0 -> "MAIN"
            1 -> "MEMORY"
            2 -> "ADJUST"
            3 -> "SKINS"
            4 -> "LEGENDARY"
            5 -> "SETTINGS"
            else -> ""
        }
    }
}
