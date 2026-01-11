import wave, struct, math, random, os
sr = 44100
os.makedirs('assets/sounds', exist_ok=True)

def save(n, s):
    with wave.open('assets/sounds/'+n, 'w') as f:
        f.setparams((1, 2, sr, 0, 'NONE', 'not compressed'))
        for v in s: f.writeframes(struct.pack('h', int(max(-1.0, min(1.0, v)) * 32767)))

def gen_stone():
    # Stone: Sharp clack (noise burst) + low thud (100Hz)
    samples = []
    for i in range(int(sr * 0.15)):
        t = i / sr
        noise = (random.random()*2-1) * math.exp(-t * 200) # Quick snap
        tone = math.sin(2 * math.pi * 100 * t) * math.exp(-t * 25) # Heavy resonance
        samples.append((noise * 0.4 + tone * 0.6))
    return samples

def gen_wood():
    # Wood: Hollow resonance (blend of 150Hz and 300Hz)
    samples = []
    for i in range(int(sr * 0.2)):
        t = i / sr
        tone = (math.sin(2 * math.pi * 150 * t) + 0.5 * math.sin(2 * math.pi * 300 * t)) * math.exp(-t * 30)
        noise = (random.random()*2-1) * 0.1 * math.exp(-t * 100)
        samples.append(tone + noise)
    return samples

def gen_grass():
    # Grass: Crispy crunch (high pass filtered noise)
    samples = []
    for i in range(int(sr * 0.1)):
        t = i / sr
        # Simulate high-pass by jittering small samples or just using high values
        noise = (random.random()*0.4 if random.random()>0.5 else -0.4) * math.exp(-t * 80)
        samples.append(noise)
    return samples

def gen_glass():
    # Glass: Metallic tink (high freq sines) + brittle noise
    samples = []
    for i in range(int(sr * 0.35)):
        t = i / sr
        tone = (math.sin(2 * math.pi * 3200 * t) + math.sin(2 * math.pi * 4800 * t)) * math.exp(-t * 12)
        noise = (random.random()*2-1) * 0.2 * math.exp(-t * 40)
        samples.append((tone + noise) * 0.5)
    return samples

def gen_liquid(type='water'):
    samples = []
    for i in range(sr * 2):
        t = i / sr
        if type == 'water':
            # Bubble: FM synthesis + soft hiss
            f = 250 + 100 * math.sin(2 * math.pi * 0.5 * t)
            tone = math.sin(2 * math.pi * f * t) * (0.5 + 0.5 * math.sin(2 * math.pi * 3 * t))
            noise = (random.random()*2-1) * 0.03
            samples.append((tone * 0.2 + noise) * 0.3)
        elif type == 'lava':
            # Lava: Low 'glop' sounds
            if random.random() < 0.002: # Glop!
                tone = math.sin(2 * math.pi * 60 * (t % 0.1)) * 0.5
            else:
                tone = math.sin(2 * math.pi * 40 * t) * 0.1
            samples.append(tone * 0.4)
        elif type == 'fire':
            if random.random() < 0.02: # Crackle
                val = (random.random()*2-1) * 0.8
            else:
                val = (random.random()*2-1) * 0.02 # Hiss
            samples.append(val * 0.4)
    return samples

save('stone_break.wav', gen_stone())
save('wood_break.wav', gen_wood())
save('grass_break.wav', gen_grass())
save('glass_break.wav', gen_glass())
save('water_ambient.wav', gen_liquid('water'))
save('lava_ambient.wav', gen_liquid('lava'))
save('fire_ambient.wav', gen_liquid('fire'))
print('All real-life texture sounds generated.')