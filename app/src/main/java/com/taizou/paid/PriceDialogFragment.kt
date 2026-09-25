package com.taizou.paid

import android.app.Dialog
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.LinearLayout
import android.widget.TextView
import androidx.fragment.app.DialogFragment
import com.google.android.material.dialog.MaterialAlertDialogBuilder

class PriceDialogFragment : DialogFragment() {

    private var selectedPlan = ""
    private var selectedPrice = 0
    private lateinit var selectedPlanText: TextView
    private lateinit var priceText: TextView
    private val allButtons = mutableListOf<Button>()

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        return MaterialAlertDialogBuilder(requireContext())
            .setView(LayoutInflater.from(requireContext()).inflate(R.layout.dialog_price, null))
            .create()
    }

    override fun onStart() {
        super.onStart()
        dialog?.window?.setBackgroundDrawableResource(android.R.color.transparent)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        selectedPlanText = view.findViewById(R.id.selectedPlan)
        priceText = view.findViewById(R.id.priceText)
        val plansContainer = view.findViewById<LinearLayout>(R.id.plansContainer)
        val buyButton = view.findViewById<Button>(R.id.buyButton)

        val plans = listOf(
            "3 DAYS" to 100,
            "7 DAYS" to 150,
            "10 DAYS" to 200,
            "15 DAYS" to 250,
            "30 DAYS" to 300,
            "60 DAYS" to 350,
            "LIFETIME ACCESS" to 450,
            "PROMO LIFETIME" to 250,
            "PROMO 60 DAYS" to 200
        )

        plans.forEach { (name, price) ->
            val isPromo = name.startsWith("PROMO")
            val btn = Button(requireContext()).apply {
                text = name
                textSize = 13f
                typeface = android.graphics.Typeface.DEFAULT_BOLD
                layoutParams = LinearLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.WRAP_CONTENT
                ).apply { setMargins(0, 0, 0, 10) }
                setUnselectedStyle(isPromo)
                allButtons.add(this)
                setOnClickListener {
                    allButtons.forEach { b ->
                        val promo = plans.any { it.first == b.text.toString() && it.first.startsWith("PROMO") }
                        b.setUnselectedStyle(promo)
                    }
                    setSelectedStyle()
                    selectedPlan = name
                    selectedPrice = price
                    selectedPlanText.text = name
                    priceText.text = "₱$price"
                }
            }
            plansContainer.addView(btn)
        }

        buyButton.setOnClickListener {
            if (selectedPrice == 0) {
                Toast.makeText(requireContext(), "Choose your plan before you buy", Toast.LENGTH_SHORT).show()
            } else {
                val intent = Intent(Intent.ACTION_VIEW, Uri.parse("https://t.me/PrimeTaizou"))
                startActivity(intent)
                dismiss()
            }
        }
    }

    private fun Button.setUnselectedStyle(isPromo: Boolean) {
        val bg = android.graphics.drawable.GradientDrawable().apply {
            setColor(0xFF2C2C3D.toInt())
            cornerRadius = 12f
            if (isPromo) {
                setStroke(3, 0xFFFF2A55.toInt())
            } else {
                setStroke(2, 0xFF555566.toInt())
            }
        }
        background = bg
        if (isPromo) {
            setTextColor(0xFFFF2A55.toInt())
        } else {
            setTextColor(0xFFFFFFFF.toInt())
        }
    }

    private fun Button.setSelectedStyle() {
        val bg = android.graphics.drawable.GradientDrawable().apply {
            setColor(0xFF3A3A50.toInt())
            cornerRadius = 12f
            setStroke(4, 0xFF00E5FF.toInt())
        }
        setTextColor(0xFF00E5FF.toInt())
        background = bg
    }
}