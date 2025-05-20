Contributing
=============

Everyone is welcome to contribute to this project.

Here are some guidelines to help you.

Special Branches
-----------------

There are three special branches in this project:

- `main`: which you get all extensively-tested features
- `unstable`: which includes beta features
- `dev`: where all changes are introduced first

From a practical point of view, you should use these branches as follows:

- Rely on `main` whenever you want to use this project in a stable environment.

- Get `unstable` whenever you want to use this project in an experimental environment.

- Use `dev` whenever you want to introduce changes to this project.

Continuous Integration (CI)
---------------------------

CI will automatically run for new PRs. Any subsequent push to the PR will cause
new instances of the pipelines to run. Apart from this, any push to the `Special
Branches` listed the in above section will new trigger CI pipeline runs.

Submitting Pull Requests
-------------------------

- Make sure that your local `dev` branch is up-to-do-date with `upstream/dev`.

- Create a branch from `dev` with your changes.

- Use naming conventions stated in this guide to name your branch.

- Add your changes to your local branch.

- Open a pull request from your branch to `dev`.

Additional Information
-----------------------

This project welcomes contributions and suggestions. Most contributions require you to
agree to a Contributor License Agreement (CLA) declaring that you have the right to,
and actually do, grant us the rights to use your contribution. For details, visit
https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need
to provide a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the
instructions provided by the bot. You will only need to do this once across all repositories using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/)
or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
