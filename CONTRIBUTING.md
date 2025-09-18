# Contributing

Thanks for your interest in contributing to OpenRTX! Due to the complexity of this project, strongly consider opening an issue to discuss your changes first, that way we can ensure that coordination with other ongoing changes and the project's roadmap. Help is always welcome, and there's plenty of room!

## Where to start

- Improving the documentation (both in this repository and in the adjacent website repository)
- Triaging issues, especially if you have the means to reproduce them and add more details to them
- Adding test coverage

In general, create a fork, get your development environment setup (we recommend VS Code with the included devcontainer), make your first build, and start exploring!

## Style Conventions

Use the `scripts/clang_format.sh` script to ensure your contributions follow the intended style conventions.

### Code Organization

The main directories for the project are described below:

```text
openrtx/     # The core program, including its UI, common utilities, and features
platform/    # Hardware specific implementations, including things like drivers for specific components, details of specific hardware targets, and low-level implementations of hardware specific needs
tests/       # Tests, both unit tests and platform tests intended to be run on device (linux or otherwise)
scripts/     # Build and utility scripts
```

More details about the individual components can be found on the [developer pages](https://openrtx.org/#/software).

### Making Changes

OpenRTX uses Git for source code management. Each commit should represent a distinct change, and as such often PRs will contain multiple commits. It isn't OK, though, for a commit to contain broken or incomplete changes. Keep your branch's revision history tidy using strategies like squashing.

Also, changes that take days of effort or thousands of lines of code should be considered a yellow flag. Consider how the change could be split up into smaller, incremental changes. That way, maintainers can provide feedback more quickly and contributors have less risk of rework.

## Pull Request Process

Once you've completed a contribution, please submit it as a pull request. Ensure that you own the intellectual property that you are contributing (or have an appropriate license).

1. Make sure your branch is up-to-date, preferring to rebase your changes rather than introduce unnecessary merge commits
2. Ensure that the PR template is completed, and don't skip sections; this helps reviewers and the community understand what changes are and how they work
3. Maintainers will review new PRs as quickly as possible, but there is no time estimate or guarantee

## Code of Conduct

### Our Pledge

We pledge to make our community welcoming, safe, and equitable for all.

We are committed to fostering an environment that respects and promotes the dignity, rights, and contributions of all individuals, regardless of characteristics including race, ethnicity, caste, color, age, physical characteristics, neurodiversity, disability, sex or gender, gender identity or expression, sexual orientation, language, philosophy or religion, national or social origin, socio-economic position, level of education, or other status. The same privileges of participation are extended to everyone who participates in good faith and in accordance with this Covenant.

### Encouraged Behaviors

While acknowledging differences in social norms, we all strive to meet our community's expectations for positive behavior. We also understand that our words and actions may be interpreted differently than we intend based on culture, background, or native language.

With these considerations in mind, we agree to behave mindfully toward each other and act in ways that center our shared values, including:

1. Respecting the **purpose of our community**, our activities, and our ways of gathering.
2. Engaging **kindly and honestly** with others.
3. Respecting **different viewpoints** and experiences.
4. **Taking responsibility** for our actions and contributions.
5. Gracefully giving and accepting **constructive feedback**.
6. Committing to **repairing harm** when it occurs.
7. Behaving in other ways that promote and sustain the **well-being of our community**.

### Restricted Behaviors

We agree to restrict the following behaviors in our community. Instances, threats, and promotion of these behaviors are violations of this Code of Conduct.

1. **Harassment.** Violating explicitly expressed boundaries or engaging in unnecessary personal attention after any clear request to stop.
2. **Character attacks.** Making insulting, demeaning, or pejorative comments directed at a community member or group of people.
3. **Stereotyping or discrimination.** Characterizing anyone’s personality or behavior on the basis of immutable identities or traits.
4. **Sexualization.** Behaving in a way that would generally be considered inappropriately intimate in the context or purpose of the community.
5. **Violating confidentiality**. Sharing or acting on someone's personal or private information without their permission.
6. **Endangerment.** Causing, encouraging, or threatening violence or other harm toward any person or group.
7. Behaving in other ways that **threaten the well-being** of our community.

#### Other Restrictions

1. **Misleading identity.** Impersonating someone else for any reason, or pretending to be someone else to evade enforcement actions.
2. **Failing to credit sources.** Not properly crediting the sources of content you contribute.
3. **Promotional materials**. Sharing marketing or other commercial content in a way that is outside the norms of the community.
4. **Irresponsible communication.** Failing to responsibly present content which includes, links or describes any other restricted behaviors.

### Reporting an Issue

Tensions can occur between community members even when they are trying their best to collaborate. Not every conflict represents a code of conduct violation, and this Code of Conduct reinforces encouraged behaviors and norms that can help avoid conflicts and minimize harm.

When an incident does occur, it is important to report it promptly. To report a possible violation, contact a maintainer.

Maintainers take reports of violations seriously and will make every effort to respond in a timely manner. They will investigate all reports of code of conduct violations, reviewing messages, logs, and recordings, or interviewing witnesses and other participants. Maintainers will keep investigation and enforcement actions as transparent as possible while prioritizing safety and confidentiality. In order to honor these values, enforcement actions are carried out in private with the involved parties, but communicating to the whole community may be part of a mutually agreed upon resolution.

### Scope

This Code of Conduct applies within all community spaces, and also applies when an individual is officially representing the community in public or other spaces. Examples of representing our community include using an official email address, posting via an official social media account, or acting as an appointed representative at an online or offline event.

### Attribution

This Code of Conduct is adapted from the Contributor Covenant, version 3.0, permanently available at [https://www.contributor-covenant.org/version/3/0/](https://www.contributor-covenant.org/version/3/0/).

Contributor Covenant is stewarded by the Organization for Ethical Source and licensed under CC BY-SA 4.0. To view a copy of this license, visit [https://creativecommons.org/licenses/by-sa/4.0/](https://creativecommons.org/licenses/by-sa/4.0/)
