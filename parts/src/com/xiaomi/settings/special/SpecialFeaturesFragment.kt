package com.xiaomi.settings.special

import android.os.Bundle
import com.android.settingslib.widget.SettingsBasePreferenceFragment
import com.xiaomi.settings.R

class SpecialFeaturesFragment : SettingsBasePreferenceFragment() {
    override fun onCreatePreferences(savedInstanceState: Bundle?, rootKey: String?) {
        setPreferencesFromResource(R.xml.special_features, rootKey)
    }
}
