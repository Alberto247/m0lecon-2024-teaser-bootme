# dedcleaner
Small container with binary to delete old files

## Usage
Dedcleaner is usually used as a sidecar container to delete old files from a volume (usually for AD CTF services). 

An example configuration may look like:
```
cleaner:
    image: c4tbuts4d/dedcleaner:latest
    restart: unless-stopped
    volumes:
      - news-storage:/news
    environment:
      - DELETE_AFTER=30m
      - SLEEP=5m
      - DIRS=/news
```

## Configuration:
All configuration is done via environment variables:

- `DELETE_AFTER` - time after which files will be deleted. Default: `30m`
- `SLEEP` - time between checks. Default: `30m`
- `DIRS` - comma-separated directories/patterns to clean. 

### Patterns and directories.
Dedcleaner uses [zglob](https://github.com/mattn/go-zglob/tree/master) to match the files to delete.
That means that you can use patterns like `DIRS=/file/**/*` to delete all files in `/file` directory and all subdirectories.
You can also use `DIRS=/file/` to delete all files in `/file` directory, but not in subdirectories.

For example, if you have a directory structure like:
```
/tmp
├── keks
    └── asd
└── uploads
    └── kekus
        └── kek
```

and `DIRS=/tmp/uploads/**/*,/tmp/keks` then dedcleaner will delete all files in `/tmp/uploads/kekus/kek`
and `/tmp/keks/asd` directories, resulting in the following structure:
```
/tmp
├── keks
└── uploads
    └── kekus 
```

**Please note that deadcleaner will not delete directories, only files.**