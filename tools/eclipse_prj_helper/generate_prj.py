#!/usr/bin/env python
###########################################################################
#
#  Copyright (c) 2013-2017, ARM Limited, All Rights Reserved
#  SPDX-License-Identifier: Apache-2.0
#
#  Licensed under the Apache License, Version 2.0 (the "License"); you may
#  not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
#  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
###########################################################################
import argparse
import os
import subprocess
import string
import xml.etree.ElementTree as ElementTree


script_dir = os.path.dirname(os.path.abspath(__file__))


def get_parser():

    parser = argparse.ArgumentParser(
        description='Eclipse project generator',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )

    parser.add_argument(
        '-w',
        '--workspace',
        required=True,
        help='Application/Example root directory for hosting Eclipse project files'
    )

    parser.add_argument(
        '-m',
        '--target',
        default='K64F',
        help='Compile target MCU. Example: K64F, NUCLEO_F401RE,NRF51822...'
    )
    parser.add_argument(
        '-t',
        '--toolchain',
        default='GCC_ARM',
        help='Compile toolchain. Example: ARM, GCC_ARM, IAR'
    )

    parser.add_argument(
        '-i',
        '--ide',
        default='eclipse_gcc_arm',
        help='IDE to create project files for. Example: UVISION4, UVISION5, GCC_ARM, IAR, COIDE'
    )

    return parser


def patch_makefile(workspace, target, toolchain, uvisor_dir, importer_dir):
    template_file = os.path.join(script_dir, 'Makefile.template')
    with open(template_file, 'rt') as fh:
        makefile_template = string.Template(fh.read())

    # overwrite generated Makefile
    with open(os.path.join(workspace, 'Makefile'), 'wt') as fh:
        fh.write(
            makefile_template.safe_substitute(
                workspace_dir=workspace,
                uvisor_dir=uvisor_dir,
                importer_dir=importer_dir,
                target=target,
                toolchain=toolchain,
                template_file=template_file
            )
        )


def get_uvisor_elf_name(uvisor_dir, target):
    if target == 'K64F':
        return os.path.join(
            uvisor_dir,
            'platform/kinetis/debug/configuration_kinetis_cortex_m4_0x1fff0000.elf'
        )
    raise Exception('unsupported target - ' + target)


def patch_debug_launcher(workspace, target, toolchain, uvisor_dir):
    old_launcher = os.path.join(workspace, target + '_pyocd_settings.launch')
    workspace_name = os.path.basename(workspace)
    new_launcher = os.path.join(
        workspace,
        'PyOCD_{target}_{name}.launch'.format(target=target, name=workspace_name)
    )
    os.rename(old_launcher, new_launcher)

    tree = ElementTree.parse(new_launcher)

    other_run_commands = tree.find(
        ".//stringAttribute[@key='ilg.gnuarmeclipse.debug.gdbjtag.pyocd.otherRunCommands']"
    )
    assert other_run_commands is not None
    other_run_commands.set(
        'value',
        'add-symbol-file {} __uvisor_main_start'.format(get_uvisor_elf_name(uvisor_dir, target))
    )

    stop_at = tree.find(".//stringAttribute[@key='org.eclipse.cdt.debug.gdbjtag.core.stopAt']")
    assert stop_at is not None
    stop_at.set('value', 'uvisor_init')

    program_name = tree.find(".//stringAttribute[@key='org.eclipse.cdt.launch.PROGRAM_NAME']")
    assert program_name is not None
    program_name.set(
        'value',
        os.path.join('BUILD', target, toolchain, workspace_name + '.elf'))

    debugger_name = tree.find(".//stringAttribute[@key='org.eclipse.cdt.dsf.gdb.DEBUG_NAME']")
    assert debugger_name is not None
    debugger_name.set('value', 'arm-none-eabi-gdb')

    tree.write(new_launcher)


def find_importer_dir(workspace):
    for directory, dirnames, _ in os.walk(workspace):
        if 'mbed-os' in dirnames:
            uvisor_dir = os.path.join(directory, 'mbed-os/features/FEATURE_UVISOR/importer')
            assert os.path.isdir(uvisor_dir), 'uvisor not deployed'
            return uvisor_dir
    raise Exception('mbed-os directory was not found under ' + workspace)


def main():
    parser = get_parser()
    args = parser.parse_args()
    workspace_path = os.path.abspath(args.workspace)
    subprocess.check_call(['mbed', 'export', '-m', args.target, '-i', args.ide], cwd=workspace_path)

    importer_dir = find_importer_dir(workspace_path)
    uvisor_dir = os.path.join(importer_dir, 'TARGET_IGNORE/uvisor')
    assert os.path.isdir(uvisor_dir)

    patch_makefile(workspace_path, args.target, args.toolchain, uvisor_dir, importer_dir)

    patch_debug_launcher(workspace_path, args.target, args.toolchain, uvisor_dir)


if __name__ == '__main__':
    main()
