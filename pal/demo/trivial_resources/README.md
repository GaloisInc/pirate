Manually Fetching PAL Resources
===============================

Basic steps to get config values in an application are as follows:

1. Include `pal/pal.h` (making sure `<repo>/pal/include` is in your include
   path).
2. Get the `pal` pipe fd using `get_pal_fd`. This returns the integer value
   of the `PAL_FD` environment variable, if it's present and well formed.
   Otherwise, it returns -1. Under normal circumstances, this is a good way
   to check if the application was launched with `pal`.
3. Get the config values you need from `pal` using `get_<type>_res`. These
   functions take a `pal` pipe fd, a name (which must be in the `ids` field
   in the config YAML), and a value of the appropriate type to fill in. They
   return 0 on success. Otherwise, they return either a negative `errno`
   value or a positive `pal` error. `pal_strerror` will return a string
   representation of a `pal` error. Currently supported types are `integer`,
   `string`, `boolean`, and `file`.
4. Link against `libpal`.

To run the application with `pal`, create a YAML file with the path to the
executable (absolute or relative to the config file's containing directory)
and stanzas for each resource, and supply it as the argument to `pal`. E.g.,
if you wanted to have a string resource called "foo" in the executable, you
might create the following config file:

```yaml
enclaves:
    - name: any_name_you_want
    - path: path/to/executable
resources:
    - name: resource_name_to_report_in_pal_errors
      type: string
      ids: [ "any_name_you_want/foo" ]
      contents:
          string_value: configuration value the executable will receive
```

Then you would run it with `pal path/to/config.yaml`. To get `pal` to print
verbose information about its operation, add `config: {"log_level":"debug"}`
at the top level.

Note that the "name" field of each resource is used only in `pal` debug
messages. The name of the resource that gets passed to `get_*_res` should
match the part of the id after the slash. The rationale for this is that
resources may have different names in different enclave executables.

Limitations
-----------

At the moment, adding new resource types is easy, but it requires modifying
the `pal` source. Adding plugin support for different resource types is a
near-term goal.

File resources currently support paths that are absolute or relative to the
working directory when `pal` is launched. This is counterintuitive. Paths
for file resources should be relative to the config file, like paths to
executables.
