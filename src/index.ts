import util from "util";
const logSettings = { showHidden: false, depth: null, colors: true };

import HuffmanCompressor from "./cowc";

const huffmanCompressor = new HuffmanCompressor();

const compression = huffmanCompressor.execute("banananana");

console.log(util.inspect(compression, logSettings));
