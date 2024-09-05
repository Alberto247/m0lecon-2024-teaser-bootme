from hashlib import sha256
from Crypto.Cipher import ChaCha20

bzImage = open("./bzImage.original", "rb").read()



key=b"7LKFYxZh0xupHMrAzp3VoWrE6B4oFFxd"
nonce=b"\x00"*12
cipher = ChaCha20.new(key=key, nonce=nonce)
bzImage = bzImage[:0x200]+cipher.encrypt(bzImage[0x200:0x4000])+bzImage[0x4000:]

header_checksum = sha256(bzImage[:0x4000]).digest()

bzImage = bzImage[:0x4000]+cipher.encrypt(bzImage[0x4000:])

print(hex(len(bzImage)-0x4000))

body_checksum = sha256(bzImage[0x4000:]).digest()


with open("./fs/boot/bzImage", "wb") as f:
    f.write(bzImage)

with open("./fs/boot/bzImage.backup", "wb") as f:
    f.write(bzImage)

print(f"header_checksum = {header_checksum}")
print(f"body_checksum = {body_checksum}")
print(f"key = {key}")

header_checksum = "BYTE header_checksum[]={"+", ".join([f"{hex(x)}" for x in header_checksum])+"};"
body_checksum = "BYTE vmlinuz_checksum[]={"+", ".join([f"{hex(x)}" for x in body_checksum])+"};"
key = "uint8_t chacha20_key[32]={"+", ".join([f"{hex(x)}" for x in key])+"};"
print(header_checksum)
print(body_checksum)
print(key)