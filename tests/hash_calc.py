import bencodepy
import hashlib

with open("debian-13.3.0-amd64-netinst.iso.torrent", "rb") as f:
    data = bencodepy.decode(f.read())

info_bytes = bencodepy.encode(data[b"info"])
info_hash = hashlib.sha1(info_bytes).hexdigest()
print(info_hash)
