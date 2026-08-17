#!/usr/bin/env python3
"""Extract deterministic connected induced subgraphs from a SPEC MCF input."""

from __future__ import annotations

import argparse
from collections import deque
from pathlib import Path


def load_input(path: Path):
    lines = path.read_text().splitlines()
    trip_count, arc_count = map(int, lines[0].split())
    trips = lines[1 : trip_count + 1]
    arcs = [tuple(map(int, line.split())) for line in lines[trip_count + 1 :]]
    if len(arcs) != arc_count:
        raise ValueError(f"header has {arc_count} arcs, parsed {len(arcs)}")
    return trips, arcs


def adjacency(node_count: int, arcs):
    adj = [set() for _ in range(node_count + 1)]
    degree = [0] * (node_count + 1)
    for tail, head, _ in arcs:
        adj[tail].add(head)
        adj[head].add(tail)
        degree[tail] += 1
        degree[head] += 1
    return adj, degree


def connected_nodes(seed: int, size: int, adj, degree):
    chosen = []
    seen = {seed}
    queue = deque([seed])
    while queue and len(chosen) < size:
        node = queue.popleft()
        chosen.append(node)
        neighbours = sorted(adj[node], key=lambda n: (-degree[n], n))
        for neighbour in neighbours:
            if neighbour not in seen:
                seen.add(neighbour)
                queue.append(neighbour)
    if len(chosen) != size:
        raise ValueError(f"seed {seed}: component only yielded {len(chosen)} nodes")
    return sorted(chosen)


def write_slice(path: Path, nodes, trips, arcs):
    renumber = {old: new for new, old in enumerate(nodes, 1)}
    selected_arcs = [
        (renumber[tail], renumber[head], cost)
        for tail, head, cost in arcs
        if tail in renumber and head in renumber
    ]
    with path.open("w") as out:
        print(len(nodes), len(selected_arcs), file=out)
        for old in nodes:
            print(trips[old - 1], file=out)
        for arc in selected_arcs:
            print(*arc, file=out)
    return len(selected_arcs)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=Path)
    parser.add_argument("output_dir", type=Path)
    parser.add_argument("--sizes", default="32,48,64,96,128")
    parser.add_argument("--regions", type=int, default=3)
    args = parser.parse_args()

    trips, arcs = load_input(args.input)
    adj, degree = adjacency(len(trips), arcs)
    ranked = sorted(range(1, len(trips) + 1), key=lambda n: (-degree[n], n))
    args.output_dir.mkdir(parents=True, exist_ok=True)

    used_seeds = set()
    for size in map(int, args.sizes.split(",")):
        made = 0
        for seed in ranked:
            if seed in used_seeds:
                continue
            nodes = connected_nodes(seed, size, adj, degree)
            region = made + 1
            path = args.output_dir / f"connected-{size}-r{region}.in"
            arc_count = write_slice(path, nodes, trips, arcs)
            print(
                f"{path.name}: seed={seed} nodes={size} arcs={arc_count} "
                f"degree={degree[seed]}"
            )
            used_seeds.add(seed)
            made += 1
            if made == args.regions:
                break


if __name__ == "__main__":
    main()
