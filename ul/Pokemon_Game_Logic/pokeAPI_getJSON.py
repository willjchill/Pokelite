import requests
import json
import time
from requests.exceptions import RequestException, Timeout, ConnectionError

# --- CONFIGURATION ---
VERSION_GROUP = "firered-leafgreen"
OUTPUT_POKEDEX = "firered_pokedex.json"
OUTPUT_MOVES = "firered_moves.json"
REQUEST_TIMEOUT = 30  # 30 second timeout for requests
MAX_RETRIES = 3
MAX_GENERATION = 3  # Only include moves from Generation 3 or earlier

# Cache to store unique moves so we don't fetch "Tackle" 100 times
unique_moves_registry = set()
move_details_cache = {}

# Hardcoded entry for Pikachu (ID 25) in case API fails
PIKACHU_HARDCODED = {
    "id": 25,
    "name": "pikachu",
    "types": ["electric"],
    "base_stats": {
        "hp": 35,
        "attack": 55,
        "defense": 40,
        "special-attack": 50,
        "special-defense": 50,
        "speed": 90
    },
    "evolution": [
        {
            "evolves_to": "raichu",
            "condition": "Use thunder-stone"
        }
    ],
    "level_up_moves": [
        {"move": "thunder-shock", "level": 1},
        {"move": "growl", "level": 1},
        {"move": "tail-whip", "level": 6},
        {"move": "thunder-wave", "level": 8},
        {"move": "quick-attack", "level": 11},
        {"move": "double-team", "level": 15},
        {"move": "slam", "level": 20},
        {"move": "thunderbolt", "level": 26},
        {"move": "agility", "level": 33},
        {"move": "thunder", "level": 41},
        {"move": "light-screen", "level": 50}
    ]
}

def clean_text(text):
    """Removes special characters like \f (form feed) and \n found in game text."""
    if not text: return ""
    return text.replace('\f', ' ').replace('\n', ' ').replace('POKéMON', 'Pokémon')

def get_evolution_data(species_url, pokemon_name):
    """Fetches the evolution chain to see how THIS pokemon evolves."""
    try:
        species_res = requests.get(species_url, timeout=REQUEST_TIMEOUT)
        if species_res.status_code != 200:
            return None
        
        species_data = species_res.json()
        if 'evolution_chain' not in species_data or not species_data['evolution_chain']:
            return None
        
        evo_chain_url = species_data['evolution_chain']['url']

        evo_res = requests.get(evo_chain_url, timeout=REQUEST_TIMEOUT)
        if evo_res.status_code != 200:
            return None
        
        evo_data = evo_res.json()
        if 'chain' not in evo_data:
            return None
        
        chain = evo_data['chain']

        # Recursive search for the current pokemon
        def find_evolution(node, target_name):
            if node['species']['name'] == target_name:
                evolutions = []
                for next_stage in node['evolves_to']:
                    # --- SAFETY FIX: Check if details exist before accessing index 0 ---
                    if not next_stage['evolution_details']:
                        continue
                        
                    details = next_stage['evolution_details'][0]
                    trigger = details['trigger']['name']
                    
                    condition = "Unknown"
                    if trigger == 'level-up':
                        condition = f"Lvl {details.get('min_level', '?')}"
                    elif trigger == 'use-item':
                        item_name = details['item']['name'] if details['item'] else "?"
                        condition = f"Use {item_name}"
                    elif trigger == 'trade':
                        condition = "Trade"
                    
                    evolutions.append({
                        "evolves_to": next_stage['species']['name'],
                        "condition": condition
                    })
                return evolutions
            
            for child in node['evolves_to']:
                result = find_evolution(child, target_name)
                if result is not None:
                    return result
            return None

        return find_evolution(chain, pokemon_name)
    except Exception as e:
        print(f"[ERROR] Fetching evolution for {pokemon_name}: {e}")
        return None

def fetch_move_details(move_name):
    """Fetches detailed stats for a specific move with retry logic."""
    if move_name in move_details_cache:
        return move_details_cache[move_name]

    url = f"https://pokeapi.co/api/v2/move/{move_name}"
    
    # Retry logic for move fetching
    retries = 0
    while retries < MAX_RETRIES:
        try:
            res = requests.get(url, timeout=REQUEST_TIMEOUT)
            
            # Handle rate limiting (429)
            if res.status_code == 429:
                retry_after = int(res.headers.get('Retry-After', 60))
                print(f"[RATE LIMITED] Waiting {retry_after} seconds before retrying {move_name}...")
                time.sleep(retry_after)
                retries += 1
                continue
            
            if res.status_code != 200:
                if retries < MAX_RETRIES - 1:
                    print(f"[WARNING] API returned {res.status_code} for move {move_name}. Retrying...")
                    time.sleep(2 ** retries)  # Exponential backoff
                    retries += 1
                    continue
                return None
            
            data = res.json()
            
            # Filter: Only include moves from Generation 3 or earlier
            if 'generation' in data and data['generation']:
                generation_name = data['generation'].get('name', '') if isinstance(data['generation'], dict) else ''
                # Extract generation number from name (e.g., "generation-i", "generation-ii", "generation-iii")
                if generation_name:
                    gen_num = None
                    if generation_name == 'generation-i':
                        gen_num = 1
                    elif generation_name == 'generation-ii':
                        gen_num = 2
                    elif generation_name == 'generation-iii':
                        gen_num = 3
                    elif generation_name == 'generation-iv':
                        gen_num = 4
                    elif generation_name == 'generation-v':
                        gen_num = 5
                    elif generation_name == 'generation-vi':
                        gen_num = 6
                    elif generation_name == 'generation-vii':
                        gen_num = 7
                    elif generation_name == 'generation-viii':
                        gen_num = 8
                    elif generation_name == 'generation-ix':
                        gen_num = 9
                    
                    # Skip moves from later generations
                    if gen_num is not None and gen_num > MAX_GENERATION:
                        print(f"[FILTERED] Skipping {move_name} (Generation {gen_num} - only Gen {MAX_GENERATION} and earlier allowed)")
                        return None
                # If generation field exists but name can't be parsed, include it anyway (better safe than sorry)
            
            break  # Success
            
        except (Timeout, ConnectionError) as e:
            if retries < MAX_RETRIES - 1:
                print(f"[WARNING] Connection error for move {move_name}: {e}. Retrying...")
                time.sleep(2 ** retries)  # Exponential backoff
                retries += 1
                continue
            return None
        except RequestException as e:
            if retries < MAX_RETRIES - 1:
                print(f"[WARNING] Request error for move {move_name}: {e}. Retrying...")
                time.sleep(2 ** retries)
                retries += 1
                continue
            return None
        except Exception as e:
            print(f"[ERROR] Unexpected error fetching move {move_name}: {e}")
            return None
    else:
        print(f"[ERROR] Failed to fetch move {move_name} after {MAX_RETRIES} attempts")
        return None

    # Find description for FireRed/LeafGreen
    description = "No description available."
    if 'flavor_text_entries' in data:
        for entry in data['flavor_text_entries']:
            # Check for FR/LG specific text, otherwise fallback to English
            if 'version_group' in entry and entry['version_group']:
                group = entry['version_group'].get('name', '')
                if (group == "firered-leafgreen" or group == "firered") and entry.get('language', {}).get('name') == "en":
                    description = clean_text(entry.get('flavor_text', ''))
                    break
        
        # If we missed FR/LG, just take the first English one
        if description == "No description available.":
            for entry in data['flavor_text_entries']:
                if entry.get('language', {}).get('name') == "en":
                    description = clean_text(entry.get('flavor_text', ''))
                    break

    # Handle null values for power and accuracy (status moves may have None)
    power = data.get('power')
    accuracy = data.get('accuracy')
    
    move_data = {
        "name": move_name,
        "type": data.get('type', {}).get('name', 'normal') if isinstance(data.get('type'), dict) else 'normal',
        "power": power,
        "accuracy": accuracy,
        "pp": data.get('pp', 20),
        "damage_class": data.get('damage_class', {}).get('name', 'status') if isinstance(data.get('damage_class'), dict) else 'status',
        "description": description
    }
    
    move_details_cache[move_name] = move_data
    return move_data

def main():
    pokedex = []
    
    print("--- PHASE 1: Fetching Pokemon Data ---")
    for dex_id in range(1, 152): # Kanto Dex 1-151
        
        # --- RETRY LOGIC START ---
        # If ID 25 fails, we try again 3 times.
        url = f"https://pokeapi.co/api/v2/pokemon/{dex_id}"
        data = None
        retries = 0
        
        while retries < MAX_RETRIES:
            try:
                res = requests.get(url, timeout=REQUEST_TIMEOUT)
                
                # Handle rate limiting (429)
                if res.status_code == 429:
                    retry_after = int(res.headers.get('Retry-After', 60))
                    print(f"[RATE LIMITED] Waiting {retry_after} seconds before retrying #{dex_id}...")
                    time.sleep(retry_after)
                    retries += 1
                    continue
                
                if res.status_code == 200:
                    try:
                        data = res.json()
                        break # Success, exit retry loop
                    except json.JSONDecodeError as e:
                        print(f"[ERROR] Invalid JSON response for #{dex_id}: {e}")
                        if retries < MAX_RETRIES - 1:
                            time.sleep(2 ** retries)
                            retries += 1
                            continue
                        data = None
                else:
                    print(f"[WARNING] API returned {res.status_code} for #{dex_id}. Retrying...")
                    time.sleep(2 ** retries)  # Exponential backoff
                    retries += 1
            except (Timeout, ConnectionError) as e:
                print(f"[WARNING] Connection error for #{dex_id}: {e}. Retrying...")
                time.sleep(2 ** retries)  # Exponential backoff
                retries += 1
            except RequestException as e:
                print(f"[WARNING] Request error for #{dex_id}: {e}. Retrying...")
                time.sleep(2 ** retries)
                retries += 1
            except Exception as e:
                print(f"[ERROR] Unexpected error for #{dex_id}: {e}")
                if retries < MAX_RETRIES - 1:
                    time.sleep(2 ** retries)
                    retries += 1
                else:
                    data = None
                    break
        
        # Use hardcoded entry for Pikachu if API fetch fails
        using_hardcoded = False
        if not data:
            if dex_id == 25:  # Pikachu
                print(f"[INFO] Using hardcoded entry for #{dex_id} Pikachu (API failed)")
                using_hardcoded = True
            else:
                print(f"[ERROR] FAILED to fetch #{dex_id} after {MAX_RETRIES} attempts. Skipping.")
                continue
        # --- RETRY LOGIC END ---

        # Handle hardcoded Pikachu entry
        if using_hardcoded and dex_id == 25:
            entry = PIKACHU_HARDCODED.copy()
            # Register moves from hardcoded entry
            for move_entry in entry.get('level_up_moves', []):
                move_name = move_entry.get('move', '')
                if move_name:
                    unique_moves_registry.add(move_name)
            pokedex.append(entry)
            # Be nice to the API
            time.sleep(0.1)
            continue  # Skip the normal processing for hardcoded entry

        name = data.get('name', f'unknown-{dex_id}')
        print(f"Processing #{dex_id} {name}...")

        # 1. Stats - Map API stat names to our format
        stats = {}
        stat_name_mapping = {
            'hp': 'hp',
            'attack': 'attack',
            'defense': 'defense',
            'special-attack': 'special-attack',
            'special-defense': 'special-defense',
            'speed': 'speed'
        }
        
        if 'stats' in data:
            for s in data['stats']:
                if isinstance(s, dict) and 'stat' in s and 'base_stat' in s:
                    api_stat_name = s['stat'].get('name', '')
                    if api_stat_name in stat_name_mapping:
                        stats[stat_name_mapping[api_stat_name]] = s['base_stat']

        # 2. Moves (FireRed Filtered)
        my_moves = []
        if 'moves' in data:
            for move_entry in data['moves']:
                if not isinstance(move_entry, dict) or 'move' not in move_entry:
                    continue
                
                move_name = move_entry.get('move', {}).get('name', '')
                if not move_name:
                    continue
                
                if 'version_group_details' in move_entry:
                    for v_detail in move_entry['version_group_details']:
                        if not isinstance(v_detail, dict):
                            continue
                        
                        vg_name = v_detail.get('version_group', {}).get('name', '') if isinstance(v_detail.get('version_group'), dict) else ''
                        learn_method = v_detail.get('move_learn_method', {}).get('name', '') if isinstance(v_detail.get('move_learn_method'), dict) else ''
                        
                        if vg_name == VERSION_GROUP and learn_method == 'level-up':
                            level = v_detail.get('level_learned_at', 1)
                            
                            my_moves.append({"move": move_name, "level": level})
                            
                            # Register this move to be fetched in Phase 2
                            unique_moves_registry.add(move_name)
        
        my_moves.sort(key=lambda x: x.get('level', 0))

        # 3. Evolution
        evo_data = None
        if 'species' in data and isinstance(data['species'], dict) and 'url' in data['species']:
            evo_data = get_evolution_data(data['species']['url'], name)

        # 4. Types
        types = []
        if 'types' in data:
            for t in data['types']:
                if isinstance(t, dict) and 'type' in t:
                    type_name = t['type'].get('name', '') if isinstance(t['type'], dict) else ''
                    if type_name:
                        types.append(type_name)

        # 5. Build Entry
        entry = {
            "id": dex_id,
            "name": name,
            "types": types,
            "base_stats": stats,
            "evolution": evo_data if evo_data else "Final Stage",
            "level_up_moves": my_moves  # Fixed: was "moveset_level_up"
        }
        pokedex.append(entry)
        
        # Be nice to the API to prevent further rate limiting
        time.sleep(0.1)

    # Save Pokedex
    try:
        with open(OUTPUT_POKEDEX, 'w', encoding='utf-8') as f:
            json.dump(pokedex, f, indent=2, ensure_ascii=False)
        print(f"Saved {OUTPUT_POKEDEX} ({len(pokedex)} Pokemon entries)")
    except IOError as e:
        print(f"[ERROR] Failed to save {OUTPUT_POKEDEX}: {e}")
        return

    print("\n--- PHASE 2: Fetching Move Details ---")
    print(f"Found {len(unique_moves_registry)} unique moves. Fetching details...")
    print(f"[NOTE] Only moves from Generation {MAX_GENERATION} or earlier will be included.")
    
    final_move_dex = {}
    filtered_count = 0
    count = 0
    for move_name in unique_moves_registry:
        count += 1
        if count % 10 == 0: print(f"Fetched {count}/{len(unique_moves_registry)} moves...")
        
        details = fetch_move_details(move_name)
        if details:
            final_move_dex[move_name] = details
        else:
            # Move was filtered out or failed to fetch
            if move_name not in move_details_cache:
                filtered_count += 1
        
        # Small sleep to be nice to the API
        time.sleep(0.1)

    # Save Moves
    try:
        with open(OUTPUT_MOVES, 'w', encoding='utf-8') as f:
            json.dump(final_move_dex, f, indent=2, ensure_ascii=False)
        print(f"Saved {OUTPUT_MOVES} ({len(final_move_dex)} move entries)")
        if filtered_count > 0:
            print(f"[INFO] {filtered_count} moves were filtered out (Generation {MAX_GENERATION + 1} or later)")
    except IOError as e:
        print(f"[ERROR] Failed to save {OUTPUT_MOVES}: {e}")
        return
    
    print("\nAll Done! You now have the Pokedex and the associated Move Database.")

if __name__ == "__main__":
    main()
