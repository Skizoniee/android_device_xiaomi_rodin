package com.xiaomi.settings.touchsampling

import android.content.pm.ApplicationInfo
import android.content.pm.PackageManager
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Toast
import androidx.fragment.app.Fragment
import androidx.preference.PreferenceManager
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.xiaomi.settings.R
import java.util.HashSet

class TouchSamplingAppRemoverFragment : Fragment() {

    private lateinit var recyclerView: RecyclerView
    private var adapter: com.xiaomi.settings.gamebar.GameBarAutoAppsAdapter? = null
    private lateinit var packageManager: PackageManager
    private var autoAppsList: MutableList<ApplicationInfo>? = null

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        return inflater.inflate(R.layout.game_bar_app_selector, container, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        recyclerView = view.findViewById(R.id.app_list)
        packageManager = requireContext().packageManager
        recyclerView.layoutManager = LinearLayoutManager(context)
        loadAutoApps()
    }

    private fun loadAutoApps() {
        val autoAppsSet = savedAutoApps
        autoAppsList = ArrayList()
        for (pkg in autoAppsSet) {
            try {
                val info = packageManager.getApplicationInfo(pkg, 0)
                autoAppsList!!.add(info)
            } catch (e: PackageManager.NameNotFoundException) {
            }
        }
        val listener = object : com.xiaomi.settings.gamebar.GameBarAutoAppsAdapter.OnAppRemoveListener {
            override fun onAppRemove(appInfo: ApplicationInfo) {
                removeAppFromAutoList(appInfo.packageName)
                val label = appInfo.loadLabel(packageManager).toString()
                Toast.makeText(context, getString(R.string.htsr_app_removed, label), Toast.LENGTH_SHORT).show()
                autoAppsList!!.remove(appInfo)
                adapter!!.notifyDataSetChanged()
            }
        }
        adapter = com.xiaomi.settings.gamebar.GameBarAutoAppsAdapter(packageManager, autoAppsList!!, listener)
        recyclerView.adapter = adapter
    }

    private val savedAutoApps: Set<String>
        get() = PreferenceManager.getDefaultSharedPreferences(requireContext())
            .getStringSet(TouchSamplingSettingsFragment.HTSR_APPS_PREF, HashSet())!!

    private fun removeAppFromAutoList(packageName: String) {
        val autoApps = HashSet(savedAutoApps)
        autoApps.remove(packageName)
        PreferenceManager.getDefaultSharedPreferences(requireContext())
            .edit().putStringSet(TouchSamplingSettingsFragment.HTSR_APPS_PREF, autoApps).apply()
    }
}
