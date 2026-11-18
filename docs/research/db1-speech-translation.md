# DB1 graphical speech translation

Eight P-chan32x32 sprite frames contain four two-part taunts:1E/1F,27/28,30/31,38/39. They are graphics inside the actor animation, not ordinary text tables. The two speech phases retain13 animation ticks each; animation directories, pointers, frame sizes, faction palettes, bubble outlines and character pixels remain unchanged.

The English3x5 lettering uses STU/PID!, NYAH/OCTO!, YOU/PERV!, HEHE/SPUD!. The source readings are baka, yaai/tako, kono/sukebe and an ambiguous ufu-style expression followed by imo. HEHE is a short adaptation of the last expression; its exact final kana and idiomatic intent remain uncertain. OCTO/SPUD preserve the literal octopus/potato taunts rather than inventing unrelated insults.

The guarded tool edits only prequalified interior lettering masks, recompresses each stream within its original capacity and updates the ROM checksum. It rejects a different source image, altered streams, output overflow and unexpected layout. Every output is newly created under private local storage; originals remain read-only. The existing engine decoder independently confirmed all eight512-byte outputs.

## Reversible language identities

Current English CRC is38B20F8F, SHA256f680a297595dee8037cdb4240b5935de0b183509a0f0fde92f0b2794b3899f42. Qualified slots are Japanese360C1B20, currentEnglish38B20F8F, suppliedv0.1C1E5C0DA, previous map-repair3BE23E09 and previous currentEnglish950A8FC3. All five prior/current inputs remain accepted. The bounded C/Python capacity is8;9 variants reject before mutation/output. SHA metadata is emitted separately from the payload struct.

Python verified all25 exact input/target reversions on private qualified images. Actual native runs from all five inputs converge to identical guest/audio state per selected English/Japanese language, with validated progress-session initialization. Synthetic C tests additionally exercise5/8 variants, byte guards, failed-target rollback and9-variant rejection.

## Native graphical qualification

Private copied executable19f22a5455aedc75f3f6d559c3df102122c414a0ae9a12d98407533008d3eb61 completed6200 frames in each language. Eight captures per language include four directions and both factions simultaneously:32 bubble views. Dumps verify type35, action0D, directions0/2/4/6 and the two expected elapsed-time windows.

Sixteen paired JP/English sprite comparisons preserve every pixel outside the lettering interior, including outline, hand, body and tail. English opaque pixels match the translated pattern and the native faction palette. This is a prepared RAM diagnostic, not a campaign playthrough. An earlier fixture used decimal-looking values parsed as hex and was corrected; its invalid-state failure is not attributed to the game.

Private evidence: local/pchan-native-proof/QUALIFICATION.md and native-pixel-qualification.json. Extracted art and derived ROM images are not committed. This feature is newer than the immutable0.1.6 package. Staff/other remaining graphical text and comprehensive scene coverage retain their separate audit status; no complete-translation percentage is claimed.
