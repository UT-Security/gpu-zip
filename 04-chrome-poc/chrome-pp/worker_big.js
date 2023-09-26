onmessage = e => {
  // e.data: { digits: Number }

  let result = BigInt(0);
  let magicNumberString = "0x";
  for (let i = 0; i < e.data.digits; i += 1) {
    magicNumberString += "f";
  }
  let magicNumber = BigInt(magicNumberString);
  while (true) {
    result += magicNumber * magicNumber;
    result -= magicNumber * magicNumber;
  }
}
