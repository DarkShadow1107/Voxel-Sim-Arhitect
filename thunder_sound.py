import wave, struct, math, random, os
sr = 44100
os.makedirs('assets/sounds', exist_ok=True)
def save(n, s):
    with wave.open('assets/sounds/'+n, 'w') as f:
        f.setparams((1, 2, sr, 0, 'NONE', 'not compressed'))
        for v in s: f.writeframes(struct.pack('h', int(max(-1, min(1, v)) * 32767)))
# Thunder: Sub-bass rumble + distorted shockwave
thunder = []
for i in range(sr * 5):
    t = i / sr
    env = math.exp(-t * 0.8)
    shock = (random.random()*2-1) * (math.exp(-t * 20)) * 0.5
    rumble = math.sin(2 * math.pi * 35 * t + math.sin(2 * math.pi * 2 * t) * 10) * env
    thunder.append((shock + rumble) * 0.8)
save('thunder.wav', thunder)
print('Updated thunder generated.')