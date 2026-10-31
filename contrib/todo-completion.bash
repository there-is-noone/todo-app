# bash completion for todo
# Installed to share/bash-completion/completions/todo by `sudo make install`.
# Task IDs are read from the hidden `todo --ids` helper (one "id<TAB>description" per line).

# Completes task IDs and shows their descriptions while there is more than one match
_todo_ids() {
    local todo_cmd=$1 cur=$2 id description
    local -a matches=()
    while IFS=$'\t' read -r id description; do
        [[ $id == "$cur"* ]] && matches+=("$id  $description")
    done < <("$todo_cmd" --ids 2>/dev/null)

    if (( ${#matches[@]} == 1 )); then
        # A single match inserts just the ID
        COMPREPLY=("${matches[0]%%  *}")
    else
        COMPREPLY=("${matches[@]}")
    fi
}

_todo() {
    local cur=${COMP_WORDS[COMP_CWORD]}
    local cmd=${COMP_WORDS[1]}
    local commands="-a --add -l --list -s --search -arch --archive -r --remove -d --done
                    -e --edit -i --info -c --count -n --notify -q --queue -u --undo -h --help"
    local priorities="low medium high"
    local topics="add remove list search archive done edit info count notify queue undo"
    local list_flags="--pending -p --done -d --high --medium --low --queued -q --archive -arch --search -s"
    COMPREPLY=()

    if (( COMP_CWORD == 1 )); then
        COMPREPLY=($(compgen -W "$commands" -- "$cur"))
        return
    fi

    case $cmd in
        -h|--help)
            (( COMP_CWORD == 2 )) && COMPREPLY=($(compgen -W "$topics" -- "$cur"))
            ;;
        -d|--done|-r|--remove|-i|--info)
            _todo_ids "${COMP_WORDS[0]}" "$cur"
            ;;
        -e|--edit)
            if (( COMP_CWORD == 2 )); then
                _todo_ids "${COMP_WORDS[0]}" "$cur"
            else
                COMPREPLY=($(compgen -W "$priorities" -- "$cur"))
            fi
            ;;
        -a|--add|-q|--queue)
            # The priority comes after the description
            (( COMP_CWORD > 2 )) && COMPREPLY=($(compgen -W "$priorities" -- "$cur"))
            ;;
        -l|--list|-arch|--archive|-A)
            # Everything after -s is free search text
            local i
            for (( i = 2; i < COMP_CWORD; i++ )); do
                [[ ${COMP_WORDS[i]} == -s || ${COMP_WORDS[i]} == --search ]] && return
            done
            [[ $cmd == -l || $cmd == --list ]] || list_flags+=" --clear"
            COMPREPLY=($(compgen -W "$list_flags" -- "$cur"))
            ;;
        -n|--notify)
            COMPREPLY=($(compgen -W "--force -f" -- "$cur"))
            ;;
    esac
}

complete -F _todo todo
