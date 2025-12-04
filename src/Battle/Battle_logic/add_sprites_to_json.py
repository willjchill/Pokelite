#!/usr/bin/env python3
"""
Script to add sprite paths to the Pokemon JSON file.
Adds a "sprite" field to each Pokemon entry with the path pattern:
":/assets/battle/{name}.png"
"""

import json
import sys
import os

def add_sprites_to_json(json_file_path):
    """Add sprite paths to each Pokemon in the JSON file."""
    
    # Read the JSON file
    with open(json_file_path, 'r', encoding='utf-8') as f:
        pokemon_list = json.load(f)
    
    # Add sprite path to each Pokemon
    for pokemon in pokemon_list:
        if 'name' in pokemon:
            name = pokemon['name']
            # Create sprite path using the Pokemon's name
            sprite_path = f":/assets/battle/{name}.png"
            pokemon['sprite'] = sprite_path
    
    # Write back to the file
    with open(json_file_path, 'w', encoding='utf-8') as f:
        json.dump(pokemon_list, f, indent=2, ensure_ascii=False)
    
    print(f"Added sprite paths to {len(pokemon_list)} Pokemon in {json_file_path}")

if __name__ == "__main__":
    # Default path
    json_file = "firered_full_pokedex.json"
    
    # Allow command line argument
    if len(sys.argv) > 1:
        json_file = sys.argv[1]
    
    if not os.path.exists(json_file):
        print(f"Error: File {json_file} not found!")
        sys.exit(1)
    
    add_sprites_to_json(json_file)

