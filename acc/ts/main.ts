import { readFileSync } from "fs";

const input: string[] = readFileSync("/dev/stdin", "utf8").split("\n");
const a: number = parseInt(input[0]);
const [b, c]: number[] = Array.from(input[1].split(" "), Number);
const s: string = input[2];

const ans: number = a + b + c;
console.log(ans);
console.log(s);
