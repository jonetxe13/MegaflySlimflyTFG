# How to release INRFlow to the public repository #

Note: This file should not be released.

#### There are two git repositories: ####

* The dev repository wtf
* The release repository inrflow

At a high level, we continue to do all our work as normal in wtf repo. For each
release we create an orphan branch (with none of our commit history) and commit
a clean working directory to it. We then push this branch (from wtf) to inrflow.
Thus, inrflow is a repo consisting of history-less branches, and each one is a
release version. There will be no merging in the release repo.

#### The procedure is as follows: ####

Let 0704c31 be a revision we want to prepare for release as version x.x.
Checkout revision 0704c31

`$ git checkout 0704c31`

This puts you in detached HEAD state. Make a new branch from it for the last bit
of polish.

`$ git checkout -b vx.x.x.dev`

Make and commit changes, then push this branch to wtf,

`$ git push`

or maybe

`$ git push origin vx.x.x.dev`

When we are ready to release vx.x it is easier to use a clean working directory,
so clone wtf to a new place. We can get away with only doing this part once if
we keep it clean.

`$ git clone git@bitbucket.org:alejandroerickson/wtf.git wtf-clean`

Add the inrflow repository as a remote server we can push to. The name of it is
arbitrary.

`$ git remote add inrflow-release https://alejandroerickson@bitbucket.org/alejandroerickson/inrflow.git`

You can check what your remotes are by doing

`$ git remote -v`

Now we want to checkout the dev branch in wtf-clean, make an orphan branch, add
all the appropriate files (e.g., not our personal directories). If we have
pushed to wtf since cloning it into wtf-clean

`$ git checkout vx.x.x.dev`

Make the release branch a new orphan branch

`$ git checkout --orphan vx.x.x`

If we didn't do so earlier, delete things that we don't want to release. Make
the only commit that will be publically visible

`$ git commit -am "vx.x.x prepared for release"`

Push branch vx.x to the public release repo

`$ git push inrflow-release vx.x.x`

Push branch to the dev repo for our own records

`$ git push origin vx.x.x`

Finally, you can update the master branch of inrflow-release by cloning it and
doing the following:

```
git checkout vx.x.x
git merge --allow-unrelated-histories -s ours master
git checkout master
git merge vx.x.x
git push [origin master]
```


You can test it all by making your own new private repo and try to "release" to it as in these instructions.
