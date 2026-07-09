#!/bin/bash

SETUP_MARKER=".cc_setup_done"

if [ ! -f "$SETUP_MARKER" ]; then
    echo "Running first-time CC setup..."

    # Fix Bluetooth
    if [ -d packages/modules/Bluetooth/.git ]; then
        cd packages/modules/Bluetooth || exit 1

        git am --abort >/dev/null 2>&1 || true

        curl -L https://github.com/halcyonproject/packages_modules_Bluetooth/commit/9a7277fbbf3c9cf4eb8a30d9358c1ada660acf58.patch | git am

        cd ../../..
    fi

    # Keys
    if [ ! -d vendor/private/keys ]; then
        git clone https://github.com/Zeydann/android_vendor_private_keys vendor/private/keys
    fi

    # IMS
    if [ -d build/soong/.git ]; then
        cd build/soong || exit 1

        git am --abort >/dev/null 2>&1 || true

        curl -L https://github.com/halcyonproject/build_soong/commit/592363152191a911961ded314a928c725c283a36.patch | git am

        cd ../..
    fi

    # Frameworks Base
    if [ -d frameworks/base/.git ]; then
        cd frameworks/base || exit 1

        git fetch https://github.com/Zeydann/frameworks_base-cc.git 16.2
        git cherry-pick a1560f533aa7a4fe2aa7af414832a3b2ea3020ea..3bb10da60f3b135b73618a0b3523746bf774de21
        git cherry-pick \
         e37bd033fb3417ce0931f118cccdef191f67ccb8 \
         434b4952c955337efee64a5fe3a782cee11defe4

        cd ../..
    fi

    # Settings
    if [ -d packages/apps/Settings/.git ]; then
        cd packages/apps/Settings || exit 1

        git fetch https://github.com/Zeydann/packages_apps_Settings-cc.git 16.2
        git cherry-pick \
         2221b81f07042f17b03503b66f2ddf5dc5ebf4cf \
         6d57e606653065bc76b6d55251f8e2131b3773ca \
         7a8639682ee29ef4c25d6fa06a3c21a1bc2b22e5 \
         cd73c90913c4aaf9a24af0c88809de55a90fbd25 \
         d7302a121a0210fd1eacb2a460046737dde9b57c \
         9bb703d9495c9c1603e149fc119a1050fb85d772 \
         6afe3e1381cf16d88be16eda21e82477fe7b6749

        cd ../../..
    fi

    # Vendor
    if [ -d vendor/circle/.git ]; then
        cd vendor/circle || exit 1

        git fetch https://github.com/Zeydann/vendor_circle.git 16.2
        git cherry-pick c4588e6d6bb4e0bb9d568e34a0067ea57ca8cce8

        cd ../..
    fi

    # Build Soong
    if [ -d build/soong/.git ]; then
        cd build/soong || exit 1

        git fetch https://github.com/Zeydann/build_soong-cc.git 16.2
        git cherry-pick 02bb283bf6907d3a2a8b4c38929943783b756802

        cd ../..
    fi

    # SEPolicy
    if [ -d device/lineage/sepolicy/.git ]; then
        cd device/lineage/sepolicy || exit 1

        git fetch https://github.com/Zeydann/device_lineage_sepolicy-cc.git 16.2
        git cherry-pick fce4bf181042fdae5b9fe1b9b9dc8016db733491

        cd ../../..

    fi

    touch "$SETUP_MARKER"

    echo "CC setup completed."
fi