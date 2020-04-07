We use Github Releases to update the docker image.

We keep a particular release called "latest", that reflects the version
of Pirate we wish external parties to use.  To update the latest version,
do the following:

* Delete the current latest release and tag from Github:
  https://github.com/GaloisInc/pirate/releases/tag/latest

  You will need first delete the release, then return to the
  latest tag and delete it.

* Create a new release called latest:
  https://github.com/GaloisInc/pirate/releases/new
  Specify "latest" as the tag, pick the branch you want
  for the release, and publish the release.

Once these two steps are performed, a Github Action will run
that builds the Docker images and uploads them to Docker hub.
Should the action fail, the problem should be fixed, and a
new release must be created using the two steps above.  We
recommend ensuring the build process succeeds before creating
a release to minimize the risk of the release process failing.