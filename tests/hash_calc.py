import bencodepy
import hashlib

with open("debian-13.2.0-arm64-netinst.iso.torrent", "rb") as f:
    data = bencodepy.decode(f.read())

info_bytes = bencodepy.encode(data[b"info"])
info_hash = hashlib.sha1(info_bytes).hexdigest()
print(info_hash)
