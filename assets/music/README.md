# Music Assets Directory

This directory contains background music for ZomChess.

## Music Tracks

### Required Music Files (OGG format)

1. **menu_theme.ogg** - Menu background music (calm, atmospheric)
2. **battle_theme.ogg** - Battle background music (intense, tactical)
3. **victory_theme.ogg** - Victory music (triumphant)
4. **defeat_theme.ogg** - Defeat music (somber)

### Music Recommendations

For a tactical zombie game like ZomChess, consider:

**Menu Music:**
- Calm, atmospheric, slightly mysterious
- Sets the tone without being distracting
- Loopable background ambience

**Battle Music:**
- Tense, driving rhythm
- Tactical and strategic feel
- Builds intensity during gameplay
- Should not overpower sound effects

**Victory Music:**
- Triumphant and satisfying
- Celebratory but not overbearing
- Short loop (30-60 seconds)

**Defeat Music:**
- Somber and reflective
- Encourages retry without being frustrating
- Simple, melancholic melody

### Music Sources

You can find suitable music from:

1. **Free Soundtrack Libraries:**
   - [FreePD](https://freepd.com/)
   - [Incompetech](https://incompetech.com/music/)
   - [YouTube Audio Library](https://www.youtube.com/audiolibrary/)
   - [SoundCloud (CC-licensed)](https://soundcloud.com/)

2. **Game Music Composers:**
   - [NCS (NoCopyrightSounds)](https://ncs.io/)
   - [Monstercat](https://www.monstercat.com/)

3. **Create Your Own:**
   - Use DAW software (Audacity, FL Studio, Ableton)
   - Focus on loopable tracks
   - Keep file size reasonable (<5MB per track)

### Converting to OGG

If you have music in other formats (MP3, WAV, etc.):

```bash
# Using ffmpeg
ffmpeg -i input.mp3 -c:a libvorbis -q:a 4 output.ogg

# Using sox
sox input.mp3 -t ogg output.ogg
```

### File Naming

- Use lowercase with underscores: `menu_theme.ogg`
- Keep filenames descriptive but short
- Use OGG Vorbis format for best SFML compatibility

### Volume Levels

All music should be normalized to approximately:
- Peak: -1dB
- RMS: -18dB to -14dB

This ensures consistent volume across all tracks and prevents clipping.
