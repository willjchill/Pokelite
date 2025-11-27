#!/usr/bin/env python3
"""
Script to update assets.qrc to include all Pokemon sprites from game_logic/sprites
"""

import os
import re

def update_resource_file():
    qrc_file = "assets.qrc"
    sprites_dir = "game_logic/sprites"
    
    # Read existing qrc file
    with open(qrc_file, 'r') as f:
        content = f.read()
    
    # Find the closing </qresource> tag
    # We'll insert sprite files before it
    
    # Get all sprite files
    sprite_files = []
    if os.path.exists(sprites_dir):
        for root, dirs, files in os.walk(sprites_dir):
            for file in files:
                if file.endswith('.png'):
                    rel_path = os.path.join(root, file)
                    # Convert to assets path format
                    # game_logic/sprites/001_bulbasaur/front.png -> assets/game_logic/sprites/001_bulbasaur/front.png
                    assets_path = "assets/" + rel_path.replace("\\", "/")
                    sprite_files.append(assets_path)
    
    sprite_files.sort()
    
    # Find insertion point (before </qresource>)
    insertion_point = content.find('    </qresource>')
    
    if insertion_point == -1:
        print("Error: Could not find </qresource> tag")
        return
    
    # Build sprite file entries
    sprite_entries = "\n".join([f'        <file>{path}</file>' for path in sprite_files])
    
    # Insert sprite entries
    new_content = content[:insertion_point] + sprite_entries + "\n" + content[insertion_point:]
    
    # Write back
    with open(qrc_file, 'w') as f:
        f.write(new_content)
    
    print(f"Added {len(sprite_files)} sprite files to {qrc_file}")

if __name__ == "__main__":
    update_resource_file()

