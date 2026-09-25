package com.taizou.paid

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.fragment.app.FragmentActivity
import androidx.fragment.app.FragmentPagerAdapter
import androidx.viewpager.widget.ViewPager

class PageAdapter(activity: FragmentActivity) : FragmentPagerAdapter(activity.supportFragmentManager, BEHAVIOR_RESUME_ONLY_CURRENT_FRAGMENT) {

    private val pageLayouts = intArrayOf(
        R.layout.page_main,
        R.layout.page_memory,
        R.layout.page_adjustable,
        R.layout.page_skins,
        R.layout.page_legendary_skins,
        R.layout.page_settings
    )

    override fun getCount(): Int = pageLayouts.size

    override fun getItem(position: Int): Fragment {
        return PageFragment.newInstance(pageLayouts[position])
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

class PageFragment : Fragment() {

    private var layoutRes: Int = 0

    companion object {
        fun newInstance(layoutRes: Int): PageFragment {
            val fragment = PageFragment()
            val args = Bundle()
            args.putInt("layout_res", layoutRes)
            fragment.arguments = args
            return fragment
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        arguments?.let { layoutRes = it.getInt("layout_res", 0) }
    }

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        return inflater.inflate(layoutRes, container, false)
    }
}