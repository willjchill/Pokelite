import requests
import json
import time
import os
from requests.exceptions import RequestException, Timeout, ConnectionError

# --- CONFIGURATION ---
VERSION_GROUP = "firered-leafgreen"
OUTPUT_POKEDEX = "firered_pokedex.json"
OUTPUT_MOVES = "firered_moves.json"
SPRITE_DIR = "sprites"  # Folder where images will be saved
REQUEST_TIMEOUT = 30
MAX_RETRIES = 3
MAX_GENERATION = 3

# Cache to store unique moves
unique_moves_registry = set()
move_details_cache = {}

# Hardcoded entry for Pikachu
PIKACHU_HARDCODED = {
    "id": 25,
    "name": "pikachu",
    "types": ["electric"],
    "base_stats": {"hp": 35, "attack": 55, "defense": 40, "special-attack": 50, "special-defense": 50, "speed": 90},
    "evolution": [{"evolves_to": "raichu", "condition": "Use thunder-stone"}],
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
    if not text: return ""
    return text.replace('\f', ' ').replace('\n', ' ').replace('POKéMON', 'Pokémon')

def download_and_save_sprites(p_id, p_name):
    """
    Downloads front/back sprites and returns the relative paths.
    """
    # Create directory: sprites/001_bulbasaur/
    folder_name = f"{p_id:03d}_{p_name}"
    full_dir_path = os.path.join(SPRITE_DIR, folder_name)
    
    if not os.path.exists(full_dir_path):
        os.makedirs(full_dir_path)

    base_url_front = f"https://raw.githubusercontent.com/PokeAPI/sprites/master/sprites/pokemon/versions/generation-iii/firered-leafgreen/{p_id}.png"
    base_url_back = f"https://raw.githubusercontent.com/PokeAPI/sprites/master/sprites/pokemon/versions/generation-iii/firered-leafgreen/back/{p_id}.png"

    paths = {"front": None, "back": None}

    # Helper to download one file
    def download_one(url, filename):
        file_path = os.path.join(full_dir_path, filename)
        # If file exists, skip download (saves time on re-runs)
        if os.path.exists(file_path):
            return file_path
            
        try:
            res = requests.get(url, timeout=10)
            if res.status_code == 200:
                with open(file_path, 'wb') as f:
                    f.write(res.content)
                return file_path
        except Exception as e:
            print(f"    [Img Error] Failed to DL {filename} for {p_name}: {e}")
        return None

    paths["front"] = download_one(base_url_front, "front.png")
    paths["back"] = download_one(base_url_back, "back.png")
    
    return paths

def get_evolution_data(species_url, pokemon_name):
    try:
        species_res = requests.get(species_url, timeout=REQUEST_TIMEOUT)
        if species_res.status_code != 200: return None
        
        species_data = species_res.json()
        if 'evolution_chain' not in species_data or not species_data['evolution_chain']: return None
        
        evo_chain_url = species_data['evolution_chain']['url']
        evo_res = requests.get(evo_chain_url, timeout=REQUEST_TIMEOUT)
        if evo_res.status_code != 200: return None
        
        evo_data = evo_res.json()
        if 'chain' not in evo_data: return None
        
        chain = evo_data['chain']

        def find_evolution(node, target_name):
            if node['species']['name'] == target_name:
                evolutions = []
                for next_stage in node['evolves_to']:
                    if not next_stage['evolution_details']: continue
                        
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
                if result is not None: return result
            return None

        return find_evolution(chain, pokemon_name)
    except Exception as e:
        print(f"[ERROR] Fetching evolution for {pokemon_name}: {e}")
        return None

def fetch_move_details(move_name):
    if move_name in move_details_cache: return move_details_cache[move_name]

    url = f"https://pokeapi.co/api/v2/move/{move_name}"
    retries = 0
    while retries < MAX_RETRIES:
        try:
            res = requests.get(url, timeout=REQUEST_TIMEOUT)
            if res.status_code == 429:
                retry_after = int(res.headers.get('Retry-After', 60))
                time.sleep(retry_after)
                retries += 1
                continue
            
            if res.status_code != 200:
                time.sleep(2 ** retries)
                retries += 1
                continue
            
            data = res.json()
            
            # Gen Filter
            if 'generation' in data and data['generation']:
                gen_name = data['generation'].get('name', '')
                gen_map = {'generation-i': 1, 'generation-ii': 2, 'generation-iii': 3, 'generation-iv': 4}
                gen_num = gen_map.get(gen_name, 99)
                if gen_num > MAX_GENERATION: return None
            
            break
        except Exception:
            time.sleep(2 ** retries)
            retries += 1
            if retries == MAX_RETRIES: return None

    description = "No description available."
    if 'flavor_text_entries' in data:
        for entry in data['flavor_text_entries']:
            group = entry.get('version_group', {}).get('name', '')
            if (group == "firered-leafgreen" or group == "firered") and entry.get('language', {}).get('name') == "en":
                description = clean_text(entry.get('flavor_text', ''))
                break
        if description == "No description available.":
             for entry in data['flavor_text_entries']:
                if entry.get('language', {}).get('name') == "en":
                    description = clean_text(entry.get('flavor_text', ''))
                    break

    move_data = {
        "name": move_name,
        "type": data.get('type', {}).get('name', 'normal'),
        "power": data.get('power'),
        "accuracy": data.get('accuracy'),
        "pp": data.get('pp', 20),
        "damage_class": data.get('damage_class', {}).get('name', 'status'),
        "description": description
    }
    move_details_cache[move_name] = move_data
    return move_data

def main():
    if not os.path.exists(SPRITE_DIR):
        os.makedirs(SPRITE_DIR)

    pokedex = []
    print("--- PHASE 1: Fetching Pokemon Data & Sprites ---")
    
    # Range 1 to 151 (Kanto)
    for dex_id in range(1, 152): 
        url = f"https://pokeapi.co/api/v2/pokemon/{dex_id}"
        data = None
        retries = 0
        
        # --- API FETCH LOOP ---
        while retries < MAX_RETRIES:
            try:
                res = requests.get(url, timeout=REQUEST_TIMEOUT)
                if res.status_code == 429:
                    time.sleep(int(res.headers.get('Retry-After', 60)))
                    continue
                if res.status_code == 200:
                    data = res.json()
                    break
                time.sleep(1)
                retries += 1
            except Exception:
                time.sleep(1)
                retries += 1
        
        # --- FALLBACK TO HARDCODED PIKACHU ---
        using_hardcoded = False
        if not data:
            if dex_id == 25:
                print(f"[INFO] Using hardcoded entry for #{dex_id} Pikachu")
                entry = PIKACHU_HARDCODED.copy()
                for move_entry in entry.get('level_up_moves', []):
                    unique_moves_registry.add(move_entry.get('move', ''))
                
                # NEW: Download sprites for Hardcoded entry
                sprite_paths = download_and_save_sprites(25, "pikachu")
                entry["sprites"] = sprite_paths
                
                pokedex.append(entry)
                continue
            else:
                print(f"[ERROR] Skipping #{dex_id}")
                continue

        name = data.get('name', f'unknown-{dex_id}')
        print(f"Processing #{dex_id} {name}...")

        # 1. Stats
        stats = {}
        stat_map = {'hp':'hp', 'attack':'attack', 'defense':'defense', 'special-attack':'special-attack', 'special-defense':'special-defense', 'speed':'speed'}
        for s in data['stats']:
            if s['stat']['name'] in stat_map:
                stats[stat_map[s['stat']['name']]] = s['base_stat']

        # 2. Moves
        my_moves = []
        if 'moves' in data:
            for m in data['moves']:
                vg = m['version_group_details']
                for v in vg:
                    if v['version_group']['name'] == VERSION_GROUP and v['move_learn_method']['name'] == 'level-up':
                        m_name = m['move']['name']
                        my_moves.append({"move": m_name, "level": v['level_learned_at']})
                        unique_moves_registry.add(m_name)
        my_moves.sort(key=lambda x: x['level'])

        # 3. Evolution
        evo_data = None
        if 'species' in data:
            evo_data = get_evolution_data(data['species']['url'], name)

        # 4. Types
        types = [t['type']['name'] for t in data['types']]

        # 5. NEW: Download Sprites
        sprite_paths = download_and_save_sprites(dex_id, name)

        # 6. Build Entry
        entry = {
            "id": dex_id,
            "name": name,
            "types": types,
            "base_stats": stats,
            "sprites": sprite_paths,  # <--- NEW FIELD
            "evolution": evo_data if evo_data else "Final Stage",
            "level_up_moves": my_moves
        }
        pokedex.append(entry)
        time.sleep(0.05)

    # Save Pokedex
    with open(OUTPUT_POKEDEX, 'w', encoding='utf-8') as f:
        json.dump(pokedex, f, indent=2)
    print(f"Saved {OUTPUT_POKEDEX} with sprites linked.")

    print("\n--- PHASE 2: Fetching Move Details ---")
    print(f"Fetching details for {len(unique_moves_registry)} unique moves...")
    
    final_move_dex = {}
    for i, move_name in enumerate(unique_moves_registry):
        if i % 20 == 0: print(f"  {i}/{len(unique_moves_registry)}...")
        details = fetch_move_details(move_name)
        if details:
            final_move_dex[move_name] = details
        time.sleep(0.05)

    with open(OUTPUT_MOVES, 'w', encoding='utf-8') as f:
        json.dump(final_move_dex, f, indent=2)
    print(f"Saved {OUTPUT_MOVES}.")

if __name__ == "__main__":
    main()
