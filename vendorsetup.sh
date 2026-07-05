#!/bin/bash

SETUP_MARKER=".cc_setup_done"

if [ ! -f "$SETUP_MARKER" ]; then
    echo "Running first-time CC setup..."

    # Fix Bluetooth
    if [ -d packages/modules/Bluetooth/.git ]; then
        cd packages/modules/Bluetooth || exit 1

        # Bersihkan kalo ada git am yang nyangkut
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

    touch "$SETUP_MARKER"

    echo "CC setup completed."
fi