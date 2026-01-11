import wave
import struct
import math
import random
import os

def generate_minecraft_music(filename, duration=180): # 3 minute tracks
    sample_rate = 44100
    num_samples = int(duration * sample_rate)
    
    with wave.open(filename, 'w') as wav_file:
        wav_file.setnchannels(1)
        wav_file.setsampwidth(2)
        wav_file.setframerate(sample_rate)
        
        # Minecraft-esque Palette: Maj7 and 9th chords
        # Chords: Fmaj7, G6, Cmaj7, Am9
        chords = [
            [174.61, 220.00, 261.63, 329.63], # Fmaj7
            [196.00, 246.94, 293.66, 392.00], # G6
            [261.63, 329.63, 392.00, 493.88], # Cmaj7
            [220.00, 261.63, 329.63, 392.00, 440.00]  # Am9
        ]
        
        # Delay line for reverb
        delay_len = int(sample_rate * 0.6)
        delay_line = [0.0] * delay_len
        delay_ptr = 0
        
        # Simple Low Pass State
        lp_state = 0.0

        for i in range(num_samples):
            t = i / sample_rate
            
            # Change chord every 8 seconds
            chord_cycle = 8.0
            chord_idx = int(t / chord_cycle) % len(chords)
            seed = chord_idx + 100 * hash(filename)
            random.seed(seed)
            current_chord = chords[chord_idx]
            
            # Evolution of the music
            # Soft pad (constant harmonic background)
            pad = 0.0
            for f in current_chord:
                pad += math.sin(2 * math.pi * f * t) * 0.05
            
            # Melodic "pings" (randomly chosen from chord)
            note_cycle = 4.0
            note_t = t % note_cycle
            note_seed = int(t / note_cycle) + seed
            random.seed(note_seed)
            active_f = random.choice(current_chord)
            
            ping_env = 0.0
            if note_t < 0.5: ping_env = note_t / 0.5 # Attack
            elif note_t < 3.0: ping_env = max(0, 1.0 - (note_t - 0.5) / 2.5) # Decay
            
            # Electric piano style (Sine + 2nd harmonic + 3rd harmonic)
            ping = (math.sin(2 * math.pi * active_f * t) * 0.6 + 
                    math.sin(2 * math.pi * active_f * 2 * t) * 0.2 + 
                    math.sin(2 * math.pi * active_f * 3 * t) * 0.1)
            
            raw_sample = pad * 0.2 + (ping * ping_env * 0.15)
            
            # Ambient nature hum (very low freq)
            raw_sample += math.sin(2 * math.pi * 40 * t) * 0.02
            
            # Delay / Reverb
            echo = delay_line[delay_ptr] * 0.4
            delay_line[delay_ptr] = raw_sample + echo
            delay_ptr = (delay_ptr + 1) % delay_len
            
            final_sample = raw_sample + echo
            
            # Simple LP filter (RC)
            lp_state = lp_state + 0.1 * (final_sample - lp_state)
            final_sample = lp_state * 0.5
            
            # Final output mix
            final_sample = max(-1.0, min(1.0, final_sample))
            packed_value = struct.pack('h', int(final_sample * 32767))
            wav_file.writeframes(packed_value)

output_dir = "assets/sounds"
if not os.path.exists(output_dir): os.makedirs(output_dir)

print("Generating high-quality background music...")
generate_minecraft_music(os.path.join(output_dir, "music1.wav"), duration=120)
generate_minecraft_music(os.path.join(output_dir, "music2.wav"), duration=120)
print("Music generation complete.")