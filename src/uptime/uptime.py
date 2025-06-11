#!/usr/bin/env python3
# Copyright 2025 AnmiTaliDev
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse
import datetime
import subprocess
import sys

def get_uptime():
    """Read system uptime from /proc/uptime"""
    try:
        with open('/proc/uptime', 'r') as f:
            return float(f.readline().split()[0])
    except Exception as e:
        sys.stderr.write(f"Error reading uptime: {e}\n")
        sys.exit(1)

def get_load_avg():
    """Read load averages from /proc/loadavg"""
    try:
        with open('/proc/loadavg', 'r') as f:
            return f.readline().split()[:3]
    except Exception as e:
        sys.stderr.write(f"Error reading load average: {e}\n")
        sys.exit(1)

def get_users():
    """Get number of unique logged-in users using 'who' command"""
    try:
        output = subprocess.check_output(['who']).decode()
        return len(set(line.split()[0] for line in output.splitlines()))
    except Exception as e:
        sys.stderr.write(f"Error getting users: {e}\n")
        sys.exit(1)

def format_uptime(seconds, pretty=False):
    """Format seconds into human-readable time components"""
    if not pretty:
        return f"{seconds:.2f} seconds"

    components = []
    days = int(seconds // 86400)
    if days > 0:
        components.append(f"{days} day{'s' if days != 1 else ''}")
    
    hours = int((seconds % 86400) // 3600)
    if hours > 0:
        components.append(f"{hours} hour{'s' if hours != 1 else ''}")
    
    minutes = int((seconds % 3600) // 60)
    if minutes > 0 or not components:
        components.append(f"{minutes} minute{'s' if minutes != 1 else ''}")
    
    return ', '.join(components)

def main():
    parser = argparse.ArgumentParser(
        description='Flexible system uptime utility',
        epilog='Part of QCO MoreUtils. Author: AnmiTaliDev. License: Apache 2.0'
    )
    parser.add_argument('-b', '--brief', action='store_true', 
                       help='Machine-friendly numerical output')
    parser.add_argument('-p', '--pretty', action='store_true',
                       help='Human-readable time format')
    parser.add_argument('-l', '--load', action='store_true',
                       help='Display load averages only')
    parser.add_argument('-u', '--uptime', action='store_true',
                       help='Display uptime only')
    parser.add_argument('-w', '--users', action='store_true',
                       help='Display user count only')
    parser.add_argument('-v', '--version', action='store_true',
                       help='Show version information')
    
    args = parser.parse_args()

    if args.version:
        print("QCO MoreUtils uptime 1.0.0\nAuthor: AnmiTaliDev\nLicense: Apache 2.0")
        return

    if not sys.platform.startswith('linux'):
        sys.stderr.write("Error: This utility requires Linux\n")
        sys.exit(1)

    # Get system data
    uptime = get_uptime()
    load = get_load_avg()
    users = get_users()

    # Handle special output modes
    if any([args.uptime, args.load, args.users]):
        outputs = []
        if args.uptime:
            outputs.append(f"{uptime:.2f}" if args.brief else 
                          format_uptime(uptime, args.pretty))
        if args.load:
            outputs.append(','.join(load) if args.brief else ' '.join(load))
        if args.users:
            outputs.append(str(users) if args.brief else f"{users} users")
        print('\n'.join(outputs))
    else:
        # Default output format
        time_str = datetime.datetime.now().strftime("%H:%M:%S")
        uptime_str = format_uptime(uptime, True)
        user_str = f"{users} user{'s' if users != 1 else ''}"
        load_str = ', '.join(load)
        print(f"{time_str} up {uptime_str}, {user_str}, load average: {load_str}")

if __name__ == '__main__':
    main()