"""Prepare version 1 Recomp Achievements event lists for the two launcher tiles."""

import argparse
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
VERSIONS = "0.5.0-beta.1, 0.5.0"
DB1_SPECIAL = {
    193201: ("Monsoon Commander", "Cast Rain successfully at least 100 times before winning a battle."),
    193202: ("Invasion Repelled", "Win a Normal, Hard or Back battle after taking out at least 100 enemies."),
    205018: ("Three Forces of Nature", "Win a battle after casting Earthquake, Tsunami and Meteor."),
    193203: ("Victory Past Zero", "Win a Normal, Hard or Back battle after time expires and triggers extinction."),
    205017: ("Rise of a King", "Evolve one of your own Tyranno into King Tyranno during a numbered stage."),
    193204: ("The PUGU Challenge", "Enter PUGU in the password menu and win the resulting battle."),
    205061: ("The PPPP Challenge", "Enter PPPP in the password menu and claim victory on that map."),
    193205: ("Clear Water Ahead", "Cast Tsunami with the battlefield clear of units to see the surfing Tyranno."),
}
DB1_CAMPAIGNS = (("Practice", 8), ("Normal", 20), ("Hard", 20), ("Back", 20))


def db1_metadata(entries):
    by_order = sorted(entries, key=lambda item: item["display_order"])
    special = by_order[:len(DB1_SPECIAL)]
    if {item["id"] for item in special} != set(DB1_SPECIAL):
        raise ValueError("DB1 special event IDs changed")
    for item in special:
        yield item, *DB1_SPECIAL[item["id"]], None
    offset = len(special)
    if len(by_order) != offset + sum(count for _, count in DB1_CAMPAIGNS):
        raise ValueError("Unexpected number of DB1 stage events")
    for campaign, count in DB1_CAMPAIGNS:
        for stage, item in enumerate(by_order[offset:offset + count], 1):
            if item["title"] != f"{campaign} {stage:02d}":
                raise ValueError(f"DB1 stage mapping changed at event {item['id']}")
            goal_type = ("win" if stage == count else "progression") if campaign == "Normal" else None
            mode = "Normal or Hard" if campaign == "Normal" else campaign
            yield item, f"{campaign} Battle {stage:02d}", f"Lead your dinosaurs to victory on {mode} stage {stage}.", goal_type
        offset += count


def db2_metadata(entries):
    for item in entries:
        goal_type = None
        description = item["description"]
        if item["kind"] == "stage" and item["mode"] == 1:
            goal_type = "win" if item["stage"] == 45 else "progression"
        elif item.get("requires_all"):
            description = description.removesuffix(".") + " in one RecompLauncher session."
        yield item, item["title"], description, goal_type


def render(game):
    catalog = ROOT / "assets" / "achievements" / f"db{game}.json"
    entries = json.loads(catalog.read_text(encoding="utf-8"))["achievements"]
    name = f"dynabrocollection--db{game}"
    achievements = []
    metadata = db1_metadata(entries) if game == 1 else db2_metadata(entries)
    for item, title, description, goal_type in metadata:
        points = max(5, item["points"])
        if points not in (5, 10, 25, 50):
            raise ValueError(f"Event {item['id']} has unsupported points")
        row = {"id": item["id"], "title": title, "description": description, "points": points, "event": f"dyna_{item['id']}"}
        if goal_type:
            row["type"] = goal_type
        achievements.append(row)
    result = {"format": 1, "game": name, "source": "events", "versions": VERSIONS, "achievements": achievements}
    return f"{name}.json", json.dumps(result, indent=2) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    destination = ROOT / "assets" / "recomp-achievements"
    for game in (1, 2):
        filename, rendered = render(game)
        path = destination / filename
        if args.check:
            if path.read_text(encoding="utf-8") != rendered:
                raise SystemExit(f"Outdated achievement list: {path}")
        else:
            destination.mkdir(parents=True, exist_ok=True)
            path.write_text(rendered, encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
