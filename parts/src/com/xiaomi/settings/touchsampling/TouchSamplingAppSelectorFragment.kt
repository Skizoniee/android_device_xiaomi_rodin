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

class TouchSamplingAppSelectorFragment : Fragment() {

    private lateinit var recyclerView: RecyclerView
    private var adapter: com.xiaomi.settings.gamebar.GameBarAppsAdapter? = null
    private lateinit var packageManager: PackageManager
    private var allApps: MutableList<ApplicationInfo>? = null

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
        loadApps()
    }

    private fun loadApps() {
        allApps = ArrayList()
        val installedApps = packageManager.getInstalledApplications(PackageManager.GET_META_DATA)
        val autoApps = savedAutoApps
        for (appInfo in installedApps) {
            if (appInfo.flags and ApplicationInfo.FLAG_SYSTEM == 0 &&
                appInfo.packageName != requireContext().packageName &&
                !autoApps.contains(appInfo.packageName)
            ) {
                allApps!!.add(appInfo)
            }
        }
        val listener = object : com.xiaomi.settings.gamebar.GameBarAppsAdapter.OnAppClickListener {
            override fun onAppClick(appInfo: ApplicationInfo) {
                addAppToAutoList(appInfo.packageName)
                val label = appInfo.loadLabel(packageManager).toString()
                Toast.makeText(context, getString(R.string.htsr_app_added, label), Toast.LENGTH_SHORT).show()
                allApps!!.remove(appInfo)
                adapter!!.notifyDataSetChanged()
            }
        }
        adapter = com.xiaomi.settings.gamebar.GameBarAppsAdapter(packageManager, allApps!!, listener)
        recyclerView.adapter = adapter
    }

    private val savedAutoApps: Set<String>
        get() = PreferenceManager.getDefaultSharedPreferences(requireContext())
            .getStringSet(TouchSamplingSettingsFragment.HTSR_APPS_PREF, HashSet())!!

    private fun addAppToAutoList(packageName: String) {
        val autoApps = HashSet(savedAutoApps)
        autoApps.add(packageName)
        PreferenceManager.getDefaultSharedPreferences(requireContext())
            .edit().putStringSet(TouchSamplingSettingsFragment.HTSR_APPS_PREF, autoApps).apply()
    }
}
