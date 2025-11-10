#!/usr/bin/env python3
"""
Quick verification script to check if all bed presence entities exist in Home Assistant.
This script can be run locally on the HA host or remotely with appropriate network access.
"""

import os
import sys
import requests
from typing import Dict, List, Optional

def get_ha_config() -> tuple[str, str]:
    """Get Home Assistant URL and token from environment or .env.local file."""
    # Try environment variables first
    ha_url = os.getenv('HA_URL')
    ha_token = os.getenv('HA_TOKEN')

    # If not in environment, try to load from .env.local
    if not ha_url or not ha_token:
        env_file = os.path.join(os.path.dirname(__file__), '..', '.env.local')
        if os.path.exists(env_file):
            with open(env_file, 'r') as f:
                for line in f:
                    line = line.strip()
                    if line.startswith('HA_URL='):
                        ha_url = line.split('=', 1)[1]
                    elif line.startswith('HA_TOKEN='):
                        ha_token = line.split('=', 1)[1]

    # Use localhost if running on HA host
    if not ha_url:
        ha_url = 'http://localhost:8123'

    if not ha_token:
        print("ERROR: HA_TOKEN not found in environment or .env.local")
        sys.exit(1)

    return ha_url, ha_token


def get_entity_state(ha_url: str, ha_token: str, entity_id: str) -> Optional[Dict]:
    """Get the state of a specific entity."""
    headers = {
        'Authorization': f'Bearer {ha_token}',
        'Content-Type': 'application/json',
    }

    try:
        response = requests.get(
            f'{ha_url}/api/states/{entity_id}',
            headers=headers,
            timeout=5
        )

        if response.status_code == 200:
            return response.json()
        elif response.status_code == 404:
            return None
        else:
            print(f"  ⚠️  HTTP {response.status_code}: {response.text}")
            return None

    except requests.exceptions.RequestException as e:
        print(f"  ❌ Connection error: {e}")
        return None


def main():
    print("=" * 70)
    print("Bed Presence Sensor - Home Assistant Entity Verification")
    print("=" * 70)

    # Get configuration
    ha_url, ha_token = get_ha_config()
    print(f"\n🔗 Connecting to: {ha_url}")

    # Define expected entities
    entities_to_check = [
        ('binary_sensor.bed_presence_detector_bed_occupied', 'Bed Occupied Binary Sensor'),
        ('sensor.bed_presence_detector_ld2410_still_energy', 'LD2410 Still Energy (primary input)'),
        ('sensor.bed_presence_detector_ld2410_moving_energy', 'LD2410 Moving Energy'),
        ('sensor.bed_presence_detector_ld2410_still_distance', 'LD2410 Still Distance'),
        ('sensor.bed_presence_detector_presence_state_reason', 'Presence State Reason Text Sensor'),
        ('sensor.bed_presence_detector_presence_change_reason', 'Presence Change Reason Text Sensor'),
        ('number.bed_presence_detector_k_on_on_threshold_multiplier', 'k_on Threshold Multiplier'),
        ('number.bed_presence_detector_k_off_off_threshold_multiplier', 'k_off Threshold Multiplier'),
        ('number.bed_presence_detector_on_debounce_ms', 'On Debounce Timer'),
        ('number.bed_presence_detector_off_debounce_ms', 'Off Debounce Timer'),
        ('number.bed_presence_detector_absolute_clear_delay_ms', 'Absolute Clear Delay'),
        ('number.bed_presence_detector_distance_min_cm', 'Distance Window Minimum'),
        ('number.bed_presence_detector_distance_max_cm', 'Distance Window Maximum'),
    ]

    print(f"\n📋 Checking {len(entities_to_check)} entities...\n")

    results = []
    for entity_id, description in entities_to_check:
        print(f"Checking: {entity_id}")
        print(f"  Description: {description}")

        state_data = get_entity_state(ha_url, ha_token, entity_id)

        if state_data:
            state = state_data.get('state', 'unknown')
            attributes = state_data.get('attributes', {})
            friendly_name = attributes.get('friendly_name', 'N/A')

            print(f"  ✅ Found: {friendly_name}")
            print(f"  📊 Current state: {state}")

            # Show additional useful info for specific entities
            if 'energy' in entity_id:
                unit = attributes.get('unit_of_measurement', '')
                print(f"  📈 Value: {state} {unit}")
            elif 'threshold' in entity_id:
                min_val = attributes.get('min', 'N/A')
                max_val = attributes.get('max', 'N/A')
                step = attributes.get('step', 'N/A')
                print(f"  🎚️  Range: {min_val} - {max_val} (step: {step})")

            results.append((entity_id, True, state))
        else:
            print(f"  ❌ NOT FOUND")
            results.append((entity_id, False, None))

        print()

    # Summary
    print("=" * 70)
    print("SUMMARY")
    print("=" * 70)

    found_count = sum(1 for _, found, _ in results if found)
    total_count = len(results)

    print(f"\n✅ Found: {found_count}/{total_count} entities")

    if found_count < total_count:
        print(f"❌ Missing: {total_count - found_count} entities")
        print("\nMissing entities:")
        for entity_id, found, _ in results:
            if not found:
                print(f"  - {entity_id}")
        return 1
    else:
        print("\n🎉 All entities verified successfully!")
        print("\nCurrent sensor readings:")
        for entity_id, found, state in results:
            if found and ('energy' in entity_id or 'occupied' in entity_id):
                print(f"  {entity_id}: {state}")
        return 0


if __name__ == '__main__':
    sys.exit(main())
