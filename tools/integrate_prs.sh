#!/usr/bin/env bash
# Integrate open PRs into work/dynabrothers one at a time: merge locally, run the
# Python suite on the result, and only then merge the PR on GitHub.
# Usage (Git Bash, dev PATH with git, gh, python): tools/integrate_prs.sh
set -u
base=work/dynabrothers
cred=(-c credential.helper= -c "credential.helper=!gh auth git-credential")
start=$(git branch --show-current)
for n in $(gh pr list --base "$base" --state open --json number -q '.[].number' | sort -n); do
  git "${cred[@]}" fetch -q origin
  git checkout -q "$base" && git reset -q --hard "origin/$base"
  br=$(gh pr view "$n" --json headRefName -q .headRefName)
  if ! git merge --no-edit -q "origin/$br" >/dev/null 2>&1; then
    echo "PR $n: conflict, left for a human/agent: $(git diff --name-only --diff-filter=U | tr '\n' ' ')"
    git merge --abort; continue
  fi
  if python -m unittest discover -s tests -p "test_*.py" >/tmp/integrate_ut.log 2>&1; then
    gh pr merge "$n" --merge >/dev/null 2>&1 && echo "PR $n: merged" || echo "PR $n: gh merge failed"
  else
    echo "PR $n: tests fail after merge, not merged"; tail -5 /tmp/integrate_ut.log
  fi
done
git checkout -q "$start"
