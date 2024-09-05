
bootloader = open("../src/bld/bootloader.bin", "rb").read()

replacements=[
    [b"ptm{n0w_w3_c4n_play_3ven_1n_4_b00tload3r!}", b"ptm{REVFLREDACTEDREDACTEDREDACTEDREDACTED}"], # rev flag
    [b"ptm{d0es_r3al_m0de_l00k_r3al_en0ugh_f0r_you?}", b"ptm{PWNFLAGREDACTEDREDACTEDREDACTEDREDACTEDR}"], # pwn flag
    [b'\x97|\xb1\x01\xca<\x92T\xce(\x08\x99\x9a\x13h\xcd&\xdc\xec\xb1T\x82 \xc1\x01\x84\x0c-:[\x00\xfa', b'\x9c\xc9>S\xe7\xd9\xeb\x9c\xce\x92\xf9\x81\x84\xf6\x8dU\x96a6\xa8\xe4\xb3~F\x06\xf8\x95\x9b\xd3\xa8\xea\x9e'], # header checksum
    [b'6\x8e\xe6\xfe\x82\xce\xfb\xaa f\x03\x1d44\xe8\xf3I).\x11\x94\xbfv\xb6y.+~\x15y\xf9`', b'W\xf1\xdb\xa5\xebq\xcf\xf0\xf1\xe4%4\x16\xa0\xa4\xae\xee\x83\xa1Y\xda\x8b\x0f\x94\x80\x04\xaa\x18\xc4\xc5\xa0f'], # body checksum
    [b'7LKFYxZh0xupHMrAzp3VoWrE6B4oFFxd', b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'], # encryption key
]

for replacement in replacements:
    original, other = replacement
    assert bootloader.count(original)==1
    assert len(original) == len(other)
    original_offset = bootloader.find(original)
    bootloader = bootloader[:original_offset]+other+bootloader[original_offset+len(original):]

with open("./bootloader.bin", "wb") as f:
    f.write(bootloader)

