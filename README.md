# Personal Repository

C_hands_on_programming_2 correspond to [C, hands-on programming 2](https://unikie.atlassian.net/browse/QA-185)
C++_hands_on_programming_RC correspond to [C++, hands-on programming 1](https://unikie.atlassian.net/browse/QA-186)


# How to clone only a part of the repo

It is not mandatory to clone all the part of the repo. You can clone only the folder corresponding to the story involved.

For instance, to clone only the part concerning [C++, hands-on programming 1](https://unikie.atlassian.net/browse/QA-186), follow these septs:

```bash
$ git init
$ git remote add origin git@github.com:RomainChaumontet/Personal
$ git config core.sparseCheckout true
$ echo "C++_hands_on_programming_RC/" >> .git/info/sparse-checkout
$ git pull --depth=1 origin master
``` 

Whit those commands, only C++_hands_on_programming_RC will be cloned.