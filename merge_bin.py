import os
Import("env")

def merge_bin(source, target, env):
    # Retrieve page offsets and file paths configured by PlatformIO
    bootloader_offset = env.subst("$BOOTLOADER_OFFSET")
    partition_table_offset = env.subst("$PARTITION_TABLE_OFFSET")
    app_offset = "0x10000" # Standard App Offset for ESP32 Arduino

    # Binaries
    bootloader_bin = env.subst("$BUILD_DIR/bootloader.bin")
    partitions_bin = env.subst("$BUILD_DIR/partitions.bin")
    boot_app0_bin = os.path.join(env.PioPlatform().get_package_dir("framework-arduinoespressif32"), "tools", "partitions", "boot_app0.bin")
    app_bin = source[0].get_abspath()

    # Output File
    merged_bin = os.path.join(env.subst("$BUILD_DIR"), "firmware_merged.bin")

    # The Flash Command (Heltec V3 is ESP32-S3)
    # Offsets: Bootloader@0x0, Partitions@0x8000, Otadata@0xE000, App@0x10000
    cmd = " ".join([
        '"$PYTHONEXE"',
        '"$OBJCOPY"',
        '--chip', 'esp32s3', 
        'merge_bin',
        '-o', f'"{merged_bin}"',
        '--flash_mode', 'dio',
        '--flash_freq', '80m',
        '--flash_size', '4MB',
        '0x0', f'"{bootloader_bin}"',
        '0x8000', f'"{partitions_bin}"',
        '0xE000', f'"{boot_app0_bin}"',
        '0x10000', f'"{app_bin}"'
    ])

    print(f"--- Merging binaries to {merged_bin} ---")
    env.Execute(cmd)

# Hook into the build process
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", merge_bin)