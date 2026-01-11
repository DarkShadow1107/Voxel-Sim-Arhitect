import wave
import struct
import math
import random
import os

def generate_noise_sound(filename, duration=0.2, freq=200):
    sample_rate = 44100
    num_samples = int(duration * sample_rate)
    with wave.open(filename, 'w') as wav_file:
        wav_file.setnchannels(1)
        wav_file.setsampwidth(2)
        wav_file.setframerate(sample_rate)
        for i in range(num_samples):
            t = i / sample_rate
            env = max(0, 1.0 - t / duration)
            noise = (random.random() * 2.0 - 1.0) * 0.4
            thump = math.sin(2 * math.pi * freq * t) * 0.6
            sample = (noise + thump) * env * 0.5
            packed_value = struct.pack('h', int(sample * 32767))
            wav_file.writeframes(packed_value)

def generate_glass_break(filename):
    sample_rate = 44100
    duration = 0.4
    num_samples = int(duration * sample_rate)
    with wave.open(filename, 'w') as wav_file:
        wav_file.setnchannels(1)
        wav_file.setsampwidth(2)
        wav_file.setframerate(sample_rate)
        for i in range(num_samples):
            t = i / sample_rate
            env = max(0, 1.0 - t / duration)
            noise = (random.random() * 2.0 - 1.0) * 0.3
            ting = math.sin(2 * math.pi * 3000 * t) * 0.4 + math.sin(2 * math.pi * 5000 * t) * 0.2
            sample = (noise + ting) * env * 0.5
            packed_value = struct.pack('h', int(sample * 32767))
            wav_file.writeframes(packed_value)

def generate_looping_ambient(filename, duration=2.0, type='water'):
    sample_rate = 44100
    num_samples = int(duration * sample_rate)
    with wave.open(filename, 'w') as wav_file:
        wav_file.setnchannels(1)
        wav_file.setsampwidth(2)
        wav_file.setframerate(sample_rate)
        for i in range(num_samples):
            t = i / sample_rate
            fade = 1.0
            if t < 0.2: fade = t / 0.2
            if t > duration - 0.2: fade = (duration - t) / 0.2
            
            if type == 'water':
                noise = (random.random() * 2.0 - 1.0) * 0.3
                low_rumble = math.sin(2 * math.pi * 50 * t) * 0.2
                sample = (noise + low_rumble) * 0.3
            elif type == 'lava':
                noise = (random.random() * 2.0 - 1.0) * 0.1
                bubbling = math.sin(2 * math.pi * 30 * t + math.sin(2 * math.pi * 2 * t) * 5)
                sample = (noise + bubbling * 0.4) * 0.4
            elif type == 'fire':
                noise = (random.random() * 2.0 - 1.0) * 0.5
                crackly = 1.0 if random.random() > 0.98 else 0.0
                sample = (noise * 0.1 + crackly * 0.4) * 0.5
            elif type == 'rain':
                sample = (random.random() * 2.0 - 1.0) * 0.2
            else:
                sample = 0
            
            packed_value = struct.pack('h', int(sample * fade * 32767))
            wav_file.writeframes(packed_value)

def generate_thunder(filename):
    sample_rate = 44100
    duration = 4.0
    num_samples = int(duration * sample_rate)
    with wave.open(filename, 'w') as wav_file:
        wav_file.setnchannels(1)
        wav_file.setsampwidth(2)
        wav_file.setframerate(sample_rate)
        for i in range(num_samples):
            t = i / sample_rate
            env = max(0, 1.0 - t / duration)
            noise = (random.random() * 2.0 - 1.0) * 0.9
            rumble = math.sin(2 * math.pi * 35 * t) * 0.6
            sample = (noise + rumble) * (env**2) * 0.7 
            # Clamp to avoid struct.error
            sample = max(-1.0, min(1.0, sample))
            packed_value = struct.pack('h', int(sample * 32767))
            wav_file.writeframes(packed_value)

output_dir = "assets/sounds"
if not os.path.exists(output_dir): os.makedirs(output_dir)

generate_noise_sound(os.path.join(output_dir, "stone_break.wav"), 0.25, 120)
generate_noise_sound(os.path.join(output_dir, "grass_break.wav"), 0.15, 300)
generate_noise_sound(os.path.join(output_dir, "wood_break.wav"), 0.3, 80)
generate_noise_sound(os.path.join(output_dir, "block_break.wav"), 0.2, 150)
generate_glass_break(os.path.join(output_dir, "glass_break.wav"))

generate_looping_ambient(os.path.join(output_dir, "water_ambient.wav"), 2.0, 'water')
generate_looping_ambient(os.path.join(output_dir, "lava_ambient.wav"), 2.0, 'lava')
generate_looping_ambient(os.path.join(output_dir, "fire_ambient.wav"), 2.0, 'fire')
generate_looping_ambient(os.path.join(output_dir, "rain_ambient.wav"), 2.0, 'rain')
generate_thunder(os.path.join(output_dir, "thunder.wav"))

print("Generated refined 16-bit sounds.")
