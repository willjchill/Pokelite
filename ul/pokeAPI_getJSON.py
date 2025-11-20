import requests
import json
import time

# --- CONFIGURATION ---
VERSION_GROUP = "firered-leafgreen"
OUTPUT_POKEDEX = "firered_pokedex.json"
OUTPUT_MOVES = "firered_moves.json"

# Cache to store unique moves so we don't fetch "Tackle" 100 times
unique_moves_registry = set()
move_details_cache = {}

def clean_text(text):
    """Removes special characters like \f (form feed) and \n found in game text."""
    return text.replace('\f', ' ').replace('\n', ' ').replace('POKéMON', 'Pokémon')

def get_evolution_data(species_url, pokemon_name):
    """Fetches the evolution chain to see how THIS pokemon evolves."""
    try:
        species_res = requests.get(species_url)
        if species_res.status_code != 200: return None
        evo_chain_url = species_res.json()['evolution_chain']['url']

        evo_res = requests.get(evo_chain_url)
        if evo_res.status_code != 200: return None
        chain = evo_res.json()['chain']

        # Recursive search for the current pokemon
        def find_evolution(node, target_name):
            if node['species']['name'] == target_name:
                evolutions = []
                for next_stage in node['evolves_to']:
                    details = next_stage['evolution_details'][0]
                    trigger = details['trigger']['name']
                    
                    condition = "Unknown"
                    if trigger == 'level-up':
                        condition = f"Lvl {details.get('min_level', '?')}"
                    elif trigger == 'use-item':
                        condition = f"Use {details['item']['name']}"
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
        print(f"Error fetching evolution for {pokemon_name}: {e}")
        return None

def fetch_move_details(move_name):
    """Fetches detailed stats for a specific move."""
    if move_name in move_details_cache:
        return move_details_cache[move_name]

    url = f"https://pokeapi.co/api/v2/move/{move_name}"
    res = requests.get(url)
    if res.status_code != 200: return None
    data = res.json()

    # Find description for FireRed/LeafGreen
    description = "No description available."
    for entry in data['flavor_text_entries']:
        # Check for FR/LG specific text, otherwise fallback to English
        group = entry['version_group']['name']
        if (group == "firered-leafgreen" or group == "firered") and entry['language']['name'] == "en":
            description = clean_text(entry['flavor_text'])
            break
    
    # If we missed FR/LG, just take the first English one
    if description == "No description available.":
        for entry in data['flavor_text_entries']:
            if entry['language']['name'] == "en":
                description = clean_text(entry['flavor_text'])
                break

    move_data = {
        "name": move_name,
        "type": data['type']['name'],
        "power": data['power'],
        "accuracy": data['accuracy'],
        "pp": data['pp'],
        "damage_class": data['damage_class']['name'], # physical/special/status
        "description": description
    }
    
    move_details_cache[move_name] = move_data
    return move_data

def main():
    pokedex = []
    
    print("--- PHASE 1: Fetching Pokémon Data ---")
    for dex_id in range(1, 152): # Kanto Dex 1-151
        res = requests.get(f"https://pokeapi.co/api/v2/pokemon/{dex_id}")
        if res.status_code != 200: continue
        data = res.json()
        name = data['name']
        
        print(f"Processing #{dex_id} {name}...")

        # 1. Stats
        stats = {s['stat']['name']: s['base_stat'] for s in data['stats']}

        # 2. Moves (FireRed Filtered)
        my_moves = []
        for move_entry in data['moves']:
            for v_detail in move_entry['version_group_details']:
                if v_detail['version_group']['name'] == VERSION_GROUP:
                    if v_detail['move_learn_method']['name'] == 'level-up':
                        move_name = move_entry['move']['name']
                        level = v_detail['level_learned_at']
                        
                        my_moves.append({"move": move_name, "level": level})
                        
                        # Register this move to be fetched in Phase 2
                        unique_moves_registry.add(move_name)
        
        my_moves.sort(key=lambda x: x['level'])

        # 3. Evolution
        evo_data = get_evolution_data(data['species']['url'], name)

        # 4. Build Entry
        entry = {
            "id": dex_id,
            "name": name,
            "types": [t['type']['name'] for t in data['types']],
            "base_stats": stats,
            "evolution": evo_data if evo_data else "Final Stage",
            "moveset_level_up": my_moves
        }
        pokedex.append(entry)

    # Save Pokedex
    with open(OUTPUT_POKEDEX, 'w') as f:
        json.dump(pokedex, f, indent=2)
    print(f"Saved {OUTPUT_POKEDEX}")

    print("\n--- PHASE 2: Fetching Move Details ---")
    print(f"Found {len(unique_moves_registry)} unique moves. Fetching details...")
    
    final_move_dex = {}
    count = 0
    for move_name in unique_moves_registry:
        count += 1
        if count % 10 == 0: print(f"Fetched {count}/{len(unique_moves_registry)} moves...")
        
        details = fetch_move_details(move_name)
        if details:
            final_move_dex[move_name] = details
        
        # Small sleep to be nice to the API
        time.sleep(0.1)

    # Save Moves
    with open(OUTPUT_MOVES, 'w') as f:
        json.dump(final_move_dex, f, indent=2)
    print(f"Saved {OUTPUT_MOVES}")
    print("\nAll Done! You now have the Pokedex and the associated Move Database.")

if __name__ == "__main__":
    main()