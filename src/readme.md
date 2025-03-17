# Can bus communication frame
| Byte | Byte[0] =  'P' / 'R' | Byte[0] =  'C' |
|----------|----------|----------|
| 0 | 'P' pressed, 'R' released| 'C' communication |
| 1 | Note 0-11 | devicePosId 0-3 |
| 2 | Octave 0-8 | 
| 3 | devicePosId 0-3 |

