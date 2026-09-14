#!/usr/bin/env python3
"""
Test suite for PicrossBoard line deduction and puzzle verification.
"""
import sys

def extract_runs(line):
    runs = []
    count = 0
    for cell in line:
        if cell == 1:
            count += 1
        else:
            if count > 0:
                runs.append(count)
                count = 0
    if count > 0:
        runs.append(count)
    return runs if runs else [0]

def is_line_satisfied(line, clues):
    clean = [c for c in clues if c > 0]
    clean_runs = [r for r in extract_runs(line) if r > 0]
    return clean == clean_runs

def deduce_line(current_line, clues):
    n = len(current_line)
    clean = [c for c in clues if c > 0]
    if not clean:
        return [2] * n # all crossed

    valid_configs = []

    def can_place(cand):
        for i in range(n):
            if current_line[i] == 1 and cand[i] != 1: return False
            if current_line[i] == 2 and cand[i] == 1: return False
        return True

    candidate = [2] * n

    def generate(clue_idx, start_pos):
        if clue_idx >= len(clean):
            if can_place(candidate):
                valid_configs.append(list(candidate))
            return

        block_len = clean[clue_idx]
        rem_space = sum(1 + clean[k] for k in range(clue_idx + 1, len(clean)))
        max_pos = n - rem_space - block_len

        for p in range(start_pos, max_pos + 1):
            if any(current_line[p + i] == 2 for i in range(block_len)):
                continue
            if (p + block_len < n) and current_line[p + block_len] == 1:
                continue

            for i in range(block_len):
                candidate[p + i] = 1

            generate(clue_idx + 1, p + block_len + 1)

            for i in range(block_len):
                candidate[p + i] = 2

    generate(0, 0)

    if not valid_configs:
        return None

    deduced = [0] * n
    for i in range(n):
        all_1 = all(cfg[i] == 1 for cfg in valid_configs)
        all_2 = all(cfg[i] == 2 for cfg in valid_configs)
        if all_1:
            deduced[i] = 1
        elif all_2:
            deduced[i] = 2
        else:
            deduced[i] = 0
    return deduced

def solve_board(w, h, row_clues, col_clues, max_steps=200):
    grid = [[0]*w for _ in range(h)]
    for step in range(max_steps):
        changed = False
        # Rows
        for r in range(h):
            cur = grid[r]
            ded = deduce_line(cur, row_clues[r])
            if ded:
                for c in range(w):
                    if cur[c] == 0 and ded[c] != 0:
                        grid[r][c] = ded[c]
                        changed = True
        # Cols
        for c in range(w):
            cur = [grid[r][c] for r in range(h)]
            ded = deduce_line(cur, col_clues[c])
            if ded:
                for r in range(h):
                    if grid[r][c] == 0 and ded[r] != 0:
                        grid[r][c] = ded[r]
                        changed = True
        if not changed:
            break

    # Check if all rows/cols satisfied
    all_sat = True
    for r in range(h):
        if not is_line_satisfied(grid[r], row_clues[r]):
            all_sat = False
    for c in range(w):
        col_line = [grid[r][c] for r in range(h)]
        if not is_line_satisfied(col_line, col_clues[c]):
            all_sat = False
    return all_sat, grid

def main():
    puzzles = [
        ("Fruit Fly", 5, 5, [
            ".#.#.",
            "#####",
            "..#..",
            ".###.",
            "..#.."
        ]),
        ("Heart SOUL", 5, 5, [
            ".#.#.",
            "#####",
            "#####",
            ".###.",
            "..#.."
        ]),
        ("Master Sword", 5, 5, [
            "..#..",
            "..#..",
            "#####",
            "..#..",
            ".###."
        ]),
        ("Nintendo Switch", 5, 5, [
            "#####",
            "#...#",
            "#...#",
            "#...#",
            "#####"
        ]),
        ("Super Mushroom", 5, 5, [
            ".###.",
            "#####",
            "#.#.#",
            ".###.",
            ".#.#."
        ]),
        ("Drosophila Larva", 10, 10, [
            "....##....",
            "...####...",
            "..##..##..",
            ".##.##.##.",
            ".########.",
            ".########.",
            "..######..",
            "...####...",
            "....##....",
            ".....#...."
        ]),
        ("Brain Connectome", 10, 10, [
            "..##..##..",
            ".########.",
            "##.####.##",
            "##########",
            "#.##..##.#",
            "####..####",
            ".########.",
            "..######..",
            "...####...",
            "....##...."
        ]),
        ("Retro Invader", 10, 10, [
            "..#....#..",
            "...#..#...",
            "..######..",
            ".##.##.##.",
            "##########",
            "#.######.#",
            "#.##..##.#",
            "...####...",
            "..##..##..",
            ".##....##."
        ]),
    ]

    print("=== Verifying Picross Puzzle Library Deductions ===")
    for title, w, h, ascii_rows in puzzles:
        sol = [[1 if ch == '#' else 2 for ch in row] for row in ascii_rows]
        row_clues = [extract_runs(sol[r]) for r in range(h)]
        col_clues = [extract_runs([sol[r][c] for r in range(h)]) for c in range(w)]

        solved, grid = solve_board(w, h, row_clues, col_clues)
        unsolved_count = sum(row.count(0) for row in grid)
        status = "SOLVED LOGICALLY" if solved and unsolved_count == 0 else f"PARTIAL ({unsolved_count} remaining)"
        print(f"[{'PASS' if solved else 'FAIL'}] {title} ({w}x{h}): {status}")

if __name__ == '__main__':
    main()
