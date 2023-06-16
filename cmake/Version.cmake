# To use this file just include it in your CMakeLists.txt via a call like this
#   include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.cmake)

# To make it work in your project, tag the current or a previous commit with a semantic version string, for example:
#   git tag v0.1.0-poc
# (both annotated and unannotated git tags work out of the box)
# You should use semantic versioning to make the parser correctly match the version numbers. This means annotating your code with a tag composed by
# the character 'v' plus 3 version numbers separated by a dot. Optional prerelease and metadata version strings can be added. More info here: https://semver.org
# The regex actually parses the 'v' as an optional so that a tag can be added without the v, but it adds it in the VERSION variable anyways.

# After including the file, the following variables are added in the global namespace:
# (given ${PROJECT_NAME} == "MYPROJ")
# - MYPROJ_VERSION: the version of the software updated at the last tag
# - MYPROJ_VERSION: the version of the software updated at the last tag together commits ahead and current commit hash
# - MYPROJ_VERSION_MAJOR: the major version of the software updated at the last tag
# - MYPROJ_VERSION_MINOR: the minor version of the software updated at the last tag
# - MYPROJ_VERSION_PATCH: the patch version of the software updated at the last tag
# - MYPROJ_VERSION_PRERELEASE: optional prerelease info (manually added to the tag after a dash, for example v1.9.4-rc1)
# - MYPROJ_VERSION_METADATA: optional metadata info (manually added to the tag after a plus, for example v1.9.4-rc1+exp.sha.5114f85)
# - MYPROJ_VERSION_COMMITS_AHEAD: the number of commits passed until the last tag was set
# - MYPROJ_VERSION_COMMIT_HASH: the hash of the current commit
# - MYPROJ_VERSION_DIRTY: set as "true" if the repository status is dirty (i.e. not all the modified tracked file have been committed), "false" otherwise

# To print all the variables at build time, define either -DCMAKE_VERSION_DEBUG=ON or -DMYPROJ_VERSION_DEBUG=ON when calling CMake.
# Notice that between to subsequent calls the variable is stored in the CMakeCache so that to remove it you should explicitly set both to OFF.

# Everything that follow are implementation details.

# The first thing to do is to check if the version was properly set in the git repository
execute_process(COMMAND bash "-c" "git describe --tags 2>&1" OUTPUT_VARIABLE ${PROJECT_NAME}_VERSION_GIT_ACTUALLY_DESCRIBES)
# Do not touch this. It's using bash -c "cd DIR; git describe --tags" because otherwise the command would not work in VSCode plugin (in which the pwd is ~)
# EDIT I didn't notice the WORKING_DIRECTORY CMake parameter! Well, this actually works so I'm not going to change it.
# This works fine on both the command line and the plugin though.
if( ${${PROJECT_NAME}_VERSION_GIT_ACTUALLY_DESCRIBES} MATCHES "fatal:.*")
    # If it is not set, prints a warning message and exits
    message("-- No tag annotations found")
    set(${PROJECT_NAME}_VERSION_MAJOR "0")
    set(${PROJECT_NAME}_VERSION_MINOR "0")
    set(${PROJECT_NAME}_VERSION_PATCH "0")
    set(${PROJECT_NAME}_VERSION_COMMITS_AHEAD "0")
    set(${PROJECT_NAME}_VERSION_DIRTY "0")
else()
    # Otherwise, the parsing of the git output begins.
    # Here using bash with a command is mandatory, because CMake would not understand the pipe.
    # If the reader wants to implement something using only CMake (without bash or sed), it's welcome. 
    execute_process(COMMAND bash "-c" "git describe --tags --abbrev=0 | sed -n 's v\\{0,1\\}\\(.*\\) v\\1 p'" OUTPUT_VARIABLE ${PROJECT_NAME}_VERSION)
    string(REGEX REPLACE "\n$" "" ${PROJECT_NAME}_VERSION "${${PROJECT_NAME}_VERSION}")

    execute_process(COMMAND bash "-c" "git describe --tags --long | sed -n 's v\\{0,1\\}\\(.*\\) v\\1 p'" OUTPUT_VARIABLE ${PROJECT_NAME}_VERSION_FULL)
    string(REGEX REPLACE "\n$" "" ${PROJECT_NAME}_VERSION_FULL "${${PROJECT_NAME}_VERSION_FULL}")

    # Adding a helper function to reduce boilerplate
    function(add_version_variable VAR_NAME GIT_CMD REGEX)
        execute_process(
            COMMAND bash "-c" "${GIT_CMD} | sed -n ${REGEX}"
            OUTPUT_VARIABLE ${PROJECT_NAME}_VERSION_${VAR_NAME}
        )
        string(REGEX REPLACE "\n$" "" ${PROJECT_NAME}_VERSION_${VAR_NAME} "${${PROJECT_NAME}_VERSION_${VAR_NAME}}")
        set(${PROJECT_NAME}_VERSION_${VAR_NAME} "${${PROJECT_NAME}_VERSION_${VAR_NAME}}" PARENT_SCOPE)
    endfunction()

    add_version_variable(
        "MAJOR"
        "git describe --tags"
        "'s v\\{0,1\\}\\([0-9]*\\).* \\1 p'"
    )

    add_version_variable(
        "MINOR"
        "git describe --tags"
        "'s v\\{0,1\\}${${PROJECT_NAME}_VERSION_MAJOR}\\.\\([0-9]*\\).* \\1 p'"
    )

    add_version_variable(
        "PATCH"
        "git describe --tags"
        "'s v\\{0,1\\}${${PROJECT_NAME}_VERSION_MAJOR}\\.${${PROJECT_NAME}_VERSION_MINOR}\\.\\([0-9]*\\).* \\1 p'"
    )

    add_version_variable(
        "PRERELEASE"
        "git describe --tags --abbrev=0"
        "'s v\\{0,1\\}${${PROJECT_NAME}_VERSION_MAJOR}\\.${${PROJECT_NAME}_VERSION_MINOR}\\.${${PROJECT_NAME}_VERSION_PATCH}-\\([\\.a-zA-Z0-9\\-]*\\).* \\1 p'"
    )

    # Here be dragons.
    # To actually parse the metadata, we have to distinguish between the case in which the prelease was present or not.
    # I actually tried to use an optional expression for the prerelease part but unfortunately it didn't work.
    # Probably a bug in the regex. Here is uselessly explicit but it does its job.
    # Notice that here we should use an escaped version of PRERELEASE, because . would match any character instead of . itself.
    # The good thing is that it matches also itself. Also, I tried with a escaped version and it didn't quite work well.
    # This works even if it's not technically correct but since it's constrained in parsing a git command no one could care a bit about correctness here.
    if("${${PROJECT_NAME}_VERSION_PRERELEASE}" STREQUAL "")
        add_version_variable(
            "METADATA"
            # The output of this git command is the last tag set with git, stripped with everything else.
            "git describe --tags --abbrev=0"
            "'s v\\{0,1\\}${${PROJECT_NAME}_VERSION_MAJOR}\\.${${PROJECT_NAME}_VERSION_MINOR}\\.${${PROJECT_NAME}_VERSION_PATCH}+\\([\\.a-zA-Z0-9\\-]*\\).* \\1 p'"
        )
    else()
        add_version_variable(
            "METADATA"
            "git describe --tags --abbrev=0"
            "'s v\\{0,1\\}${${PROJECT_NAME}_VERSION_MAJOR}\\.${${PROJECT_NAME}_VERSION_MINOR}\\.${${PROJECT_NAME}_VERSION_PATCH}-${${PROJECT_NAME}_VERSION_PRERELEASE}+\\([\\.a-zA-Z0-9\\-]*\\).* \\1 p'"
        )
    endif()

    # Same as above. Prerelease and metadata have to be stripped first because they may contain a dash that will make it difficult to parse the actual commits-ahead separator
    if("${${PROJECT_NAME}_VERSION_PRERELEASE}" STREQUAL "")
        if("${${PROJECT_NAME}_VERSION_METADATA}" STREQUAL "")
            add_version_variable(
                "COMMITS_AHEAD"
                    # The output of this git command is v`major`.`minor`.`patch`(-`prerel`)?(+`meta`)?-`commits-ahead`-`commit-hash`
                    "git describe --tags --long"
                    "'s v\\{0,1\\}${${PROJECT_NAME}_VERSION_MAJOR}\\.${${PROJECT_NAME}_VERSION_MINOR}\\.${${PROJECT_NAME}_VERSION_PATCH}-\\([0-9]*\\).* \\1 p'"
                )
        else()
            add_version_variable(
                "COMMITS_AHEAD"
                "git describe --tags --long"
                "'s v\\{0,1\\}${${PROJECT_NAME}_VERSION_MAJOR}\\.${${PROJECT_NAME}_VERSION_MINOR}\\.${${PROJECT_NAME}_VERSION_PATCH}+${${PROJECT_NAME}_VERSION_METADATA}-\\([0-9]*\\).* \\1 p'"
            )
        endif()
    else()
        if("${${PROJECT_NAME}_VERSION_METADATA}" STREQUAL "")
            add_version_variable(
                "COMMITS_AHEAD"
                "git describe --tags --long"
                "'s v\\{0,1\\}${${PROJECT_NAME}_VERSION_MAJOR}\\.${${PROJECT_NAME}_VERSION_MINOR}\\.${${PROJECT_NAME}_VERSION_PATCH}-${${PROJECT_NAME}_VERSION_PRERELEASE}-\\([0-9]*\\).* \\1 p'"
            )
        else()
            add_version_variable(
                "COMMITS_AHEAD"
                "git describe --tags --long"
                "'s v\\{0,1\\}${${PROJECT_NAME}_VERSION_MAJOR}\\.${${PROJECT_NAME}_VERSION_MINOR}\\.${${PROJECT_NAME}_VERSION_PATCH}-${${PROJECT_NAME}_VERSION_PRERELEASE}+${${PROJECT_NAME}_VERSION_METADATA}-\\([0-9]*\\).* \\1 p'"
            )
        endif()
    endif()
    
    # Oh boy. Fortunately with have only two cases.
    if("${${PROJECT_NAME}_VERSION_PRERELEASE}" STREQUAL "")
        if("${${PROJECT_NAME}_VERSION_METADATA}" STREQUAL "")
            add_version_variable(
                "COMMIT_HASH"
                "git describe --tags --long"
                "'s v\\{0,1\\}${${PROJECT_NAME}_VERSION_MAJOR}\\.${${PROJECT_NAME}_VERSION_MINOR}\\.${${PROJECT_NAME}_VERSION_PATCH}-${${PROJECT_NAME}_VERSION_COMMITS_AHEAD}-g\\([a-zA-Z0-9-]*\\).* \\1 p'"
            )
        else()
            add_version_variable(
                "COMMIT_HASH"
                "git describe --tags --long"
                "'s v\\{0,1\\}${${PROJECT_NAME}_VERSION_MAJOR}\\.${${PROJECT_NAME}_VERSION_MINOR}\\.${${PROJECT_NAME}_VERSION_PATCH}+${${PROJECT_NAME}_VERSION_METADATA}-${${PROJECT_NAME}_VERSION_COMMITS_AHEAD}-g\\([a-zA-Z0-9-]*\\).* \\1 p'"
            )
        endif()
    else()
        if("${${PROJECT_NAME}_VERSION_METADATA}" STREQUAL "")
            add_version_variable(
                "COMMIT_HASH"
                "git describe --tags --long"
                "'s v\\{0,1\\}${${PROJECT_NAME}_VERSION_MAJOR}\\.${${PROJECT_NAME}_VERSION_MINOR}\\.${${PROJECT_NAME}_VERSION_PATCH}-${${PROJECT_NAME}_VERSION_PRERELEASE}-${${PROJECT_NAME}_VERSION_COMMITS_AHEAD}-g\\([a-zA-Z0-9-]*\\).* \\1 p'"
            )
        else()
            add_version_variable(
                "COMMIT_HASH"
                "git describe --tags --long"
                "'s v\\{0,1\\}${${PROJECT_NAME}_VERSION_MAJOR}\\.${${PROJECT_NAME}_VERSION_MINOR}\\.${${PROJECT_NAME}_VERSION_PATCH}-${${PROJECT_NAME}_VERSION_PRERELEASE}+${${PROJECT_NAME}_VERSION_METADATA}-${${PROJECT_NAME}_VERSION_COMMITS_AHEAD}-g\\([a-zA-Z0-9-]*\\).* \\1 p'"
            )
        endif()
    endif()

    # The last parsing is actually the easiest. Just check if git describe with the --dirty option produces a string that contains "dirty",
    # and convert that to a boolean value (1/0 instead of true/false because for example in Python we have True/False... if
    # we want to configure a file based on version strings then it would be a problem, 1/0 is the most portable)
    add_version_variable(
        "DIRTY"
        "git describe --tags --dirty"
        "'s .*dirty.* 1 p'"
    )
    if("${${PROJECT_NAME}_VERSION_DIRTY}" STREQUAL "")
    set(${PROJECT_NAME}_VERSION_DIRTY "0")
    endif()
endif()

# We don't want to pollute the global namespace, right?
unset(${PROJECT_NAME}_VERSION_GIT_ACTUALLY_DESCRIBES)

# To explicitly print the values here, set one of the variables listed to 1, ON, TRUE, true or similar
if("${PROJECT_NAME}_VERSION_DEBUG" OR "${CMAKE_VERSION_DEBUG}")
    message("${PROJECT_NAME}_VERSION: ${${PROJECT_NAME}_VERSION}")
    message("${PROJECT_NAME}_VERSION_FULL: ${${PROJECT_NAME}_VERSION_FULL}")
    message("${PROJECT_NAME}_VERSION_MAJOR: ${${PROJECT_NAME}_VERSION_MAJOR}")
    message("${PROJECT_NAME}_VERSION_MINOR: ${${PROJECT_NAME}_VERSION_MINOR}")
    message("${PROJECT_NAME}_VERSION_PATCH: ${${PROJECT_NAME}_VERSION_PATCH}")
    message("${PROJECT_NAME}_VERSION_PRERELEASE: ${${PROJECT_NAME}_VERSION_PRERELEASE}")
    message("${PROJECT_NAME}_VERSION_METADATA: ${${PROJECT_NAME}_VERSION_METADATA}")
    message("${PROJECT_NAME}_VERSION_COMMITS_AHEAD: ${${PROJECT_NAME}_VERSION_COMMITS_AHEAD}")
    message("${PROJECT_NAME}_VERSION_COMMIT_HASH: ${${PROJECT_NAME}_VERSION_COMMIT_HASH}")
    message("${PROJECT_NAME}_VERSION_DIRTY: ${${PROJECT_NAME}_VERSION_DIRTY}")
endif()

# Tests, tried on CYCLONE project:
# git tag v1.0.0-rc1-data+exp.sha-1.12121092
# cmake .. -DCMAKE_VERSION_DEBUG=ON
#   CYCLONE_VERSION: v1.0.0-rc1-data+exp.sha-1.12121092
#   CYCLONE_VERSION_FULL: v1.0.0-rc1-data+exp.sha-1.12121092-0-g7afeeee
#   CYCLONE_VERSION_MAJOR: 1
#   CYCLONE_VERSION_MINOR: 0
#   CYCLONE_VERSION_PATCH: 0
#   CYCLONE_VERSION_PRERELEASE: rc1-data
#   CYCLONE_VERSION_METADATA: exp.sha-1.12121092
#   CYCLONE_VERSION_COMMITS_AHEAD: 0
#   CYCLONE_VERSION_COMMIT_HASH: 7afeeee
#   CYCLONE_VERSION_DIRTY: 1
# git add ../cmake/config.h.in
# git commit -m "To be reset to HEAD^"
# cmake ..
#   CYCLONE_VERSION: v1.0.0-rc1-data+exp.sha-1.12121092
#   CYCLONE_VERSION_FULL: v1.0.0-rc1-data+exp.sha-1.12121092-1-ga83cbe4
#   CYCLONE_VERSION_MAJOR: 1
#   CYCLONE_VERSION_MINOR: 0
#   CYCLONE_VERSION_PATCH: 0
#   CYCLONE_VERSION_PRERELEASE: rc1-data
#   CYCLONE_VERSION_METADATA: exp.sha-1.12121092
#   CYCLONE_VERSION_COMMITS_AHEAD: 1
#   CYCLONE_VERSION_COMMIT_HASH: a83cbe4
#   CYCLONE_VERSION_DIRTY: 1
# git tag v1.1.0
# cmake ..
#   CYCLONE_VERSION: v1.1.0
#   CYCLONE_VERSION_FULL: v1.1.0-0-ga83cbe4
#   CYCLONE_VERSION_MAJOR: 1
#   CYCLONE_VERSION_MINOR: 1
#   CYCLONE_VERSION_PATCH: 0
#   CYCLONE_VERSION_PRERELEASE: 
#   CYCLONE_VERSION_METADATA: 
#   CYCLONE_VERSION_COMMITS_AHEAD: 0
#   CYCLONE_VERSION_COMMIT_HASH: a83cbe4
#   CYCLONE_VERSION_DIRTY: 1
# git tag -d `git describe --tags`
# git reset HEAD^
# git tag -d `git describe --tags`
