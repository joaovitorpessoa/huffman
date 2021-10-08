interface IHuffmanNode {
  char: string;
  frequency: number;
}

interface IHuffmanTree {
  initialNodes: IHuffmanNode[];
  tree: any;
}

class HuffmanCompressor {
  huffmanTree: IHuffmanTree = { initialNodes: [], tree: "" };

  registerInitialNode(char: string): void {
    this.huffmanTree.initialNodes.push({ char, frequency: 1 });
  }

  sortInitialNodes(): void {
    /**
     * ordena pela frequencia, se tiver frequencia repetida, ordena pela char
     */

    const priorityQueue: IHuffmanNode[] = [];

    this.huffmanTree.initialNodes.forEach((node) => {
      const index = priorityQueue.findIndex((item) => item.char === node.char);

      priorityQueue.at();
    });
  }

  discoverInitialNodes(text: string): void {
    for (let char = 0; char < text.length; char++) {
      const charAlreadyRegister = this.huffmanTree.initialNodes.find(
        (item) => item.char === text[char]
      );

      charAlreadyRegister
        ? charAlreadyRegister.frequency++
        : this.registerInitialNode(text[char]);
    }

    this.sortInitialNodes();
  }

  execute(text: string) {
    this.discoverInitialNodes(text);
  }
}

const huffmanCompressor = new HuffmanCompressor();

// const output = huffmanCompressor.getFrequency("soooss");

// console.log(" ".charCodeAt(0));
// console.log(output);
/**
 * Iterar na string;
 * Pesquisa se já tem o char no array, se tiver, incrementa a frequency dele
 * Após término da string, dá sorte no array considerando frequency e valor do char segundo tabela ascii (.charCodeAt)
 */
