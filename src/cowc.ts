interface IChar {
  char: string;
  ascii: number;
  frequency: number;
  tree?: object;
}

class HuffmanCompressor {
  chars: IChar[] = [];

  execute(phrase: string) {
    this.discoverCharFrequency(phrase);
    this.sortByFrequency();
    this.compress();

    return this.chars[0];
  }

  discoverCharFrequency(phrase: string) {
    phrase
      .split("")
      .forEach((char) =>
        this.isCharAlreadyRegistered(char)
          ? this.incrementFrequency(char)
          : this.registerChar(char)
      );
  }

  registerChar(char: string) {
    this.chars.push({ char, ascii: char.charCodeAt(0), frequency: 1 });
  }

  isCharAlreadyRegistered(char: string) {
    for (const { ascii } of this.chars) {
      if (ascii === char.charCodeAt(0)) {
        return true;
      }
    }
    return false;
  }

  incrementFrequency(char: string) {
    this.findChar(char).frequency++;
  }

  findChar(target: string) {
    const [char] = this.chars.filter(({ char }) => char == target);
    return char;
  }

  sortByFrequency() {
    this.chars.sort(({ frequency: a }, { frequency: b }) => a - b);
  }

  compress() {
    const FIRST_ITEM_OF_ARRAY = this.chars[0];
    const SECOND_ITEM_OF_ARRAY = this.chars[1];

    this.chars.push({
      char: "+",
      frequency: FIRST_ITEM_OF_ARRAY.frequency + SECOND_ITEM_OF_ARRAY.frequency,
      ascii: "+".charCodeAt(0),
      tree: {
        [FIRST_ITEM_OF_ARRAY.char]: FIRST_ITEM_OF_ARRAY,
        [SECOND_ITEM_OF_ARRAY.char]: SECOND_ITEM_OF_ARRAY,
      },
    });

    this.removeTheFirstNChars(2);
    this.sortByFrequency();

    if (this.chars.length !== 1) {
      this.compress();
    }
  }

  removeTheFirstNChars(n: number) {
    this.chars.splice(0, n);
  }
}

export default HuffmanCompressor;
