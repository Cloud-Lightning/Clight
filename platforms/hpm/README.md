# HPM Platform Template

The public repository does not include the generated `hpm5e31_lqfp` board package.

To build on HPM:

1. Install and configure the HPM SDK locally.
2. Use the HPM VSCode pin assignment and board tools to generate a board package.
3. Put the generated files in `platforms/hpm/boards/<board_name>/`.
4. Create or update the product CMake to select `<board_name>`.
5. Keep `.hpmpc`, build output, SDK mirrors, and credentials out of Git.

The `main` folder is only an empty entry template.
