import hashlib
from typing import Iterable, List, Optional, Tuple, Union

BECH32A_ALPHABET = "qpzry9x8gf2tvdw0s3jn54khce6mua7l"

def convertbits(data: Iterable[int], frombits: int, tobits: int, pad: bool = True) -> Optional[List[int]]:
    """General power-of-2 base conversion."""
    acc = 0
    bits = 0
    ret = []
    maxv = (1 << tobits) - 1
    max_acc = (1 << (frombits + tobits - 1)) - 1

    print(f"maxv: {maxv}")
    print(f"max_acc: {max_acc}")

    for value in data:
        if value < 0 or (value >> frombits):
            return None
        acc = ((acc << frombits) | value) & max_acc
        bits += frombits
        while bits >= tobits:
            bits -= tobits
            ret.append((acc >> bits) & maxv)
    if pad:
        if bits:
            ret.append((acc << (tobits - bits)) & maxv)
    elif bits >= frombits or ((acc << (tobits - bits)) & maxv):
        return None
    return ret

def bech32_polymod(values):
  GEN = [0x3b6a57b2, 0x26508e6d, 0x1ea119fa, 0x3d4233dd, 0x2a1462b3]
  chk = 1
  for v in values:
    b = (chk >> 25)
    chk = (chk & 0x1ffffff) << 5 ^ v
    for i in range(5):
      chk ^= GEN[i] if ((b >> i) & 1) else 0
  return chk

def bech32_hrp_expand(s):
  return [ord(x) >> 5 for x in s] + [0] + [ord(x) & 31 for x in s]

def bech32_verify_checksum(hrp, data):
    polymod = bech32_polymod(bech32_hrp_expand(hrp) + data)
    print("polymod", polymod)
    return  polymod == 1

def bech32_create_checksum(hrp, data):
    values = bech32_hrp_expand(hrp) + data
    polymod = bech32_polymod(values + [0,0,0,0,0,0]) ^ 1
    return [(polymod >> 5 * (5 - i)) & 31 for i in range(6)]


pubkey = 0x0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798


sha256 = hashlib.sha256()
sha256.update(pubkey.to_bytes(length=33))

sha265_pubkey = sha256.hexdigest()
print(sha265_pubkey)

ripemd160 = hashlib.new('ripemd160')
ripemd160.update(sha256.digest())
riped_sha256_pubkey = ripemd160.digest()
print(ripemd160.hexdigest())

squashed = convertbits(riped_sha256_pubkey, 8, 5)
squashed.insert(0, 0)

checksum = bech32_create_checksum(hrp="bc", data=squashed)
print(checksum)
squashed.extend(checksum)

encoded = ""
print(squashed)
for i in squashed:
    encoded += BECH32A_ALPHABET[i]

bech32_addr = "bc" + "1" + encoded
print(bech32_addr)
assert bech32_addr == "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4"