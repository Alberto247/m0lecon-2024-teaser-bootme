package main

import (
	"fmt"
	"os"
	"os/signal"
	"path/filepath"
	"strings"
	"syscall"
	"time"

	"github.com/mattn/go-zglob"
	"github.com/sirupsen/logrus"
)

var (
	deleteAfter time.Duration
	sleep       time.Duration
	directories []string
)

func resolve(pattern string) ([]string, error) {
	entries, err := zglob.Glob(pattern)
	if err != nil {
		return nil, fmt.Errorf("failed to resolve pattern '%s': %w", pattern, err)
	}

	// Path to directory is provided, no pattern.
	if len(entries) == 1 && entries[0] == pattern {
		dirEntries, err := os.ReadDir(pattern)
		if err != nil {
			return nil, fmt.Errorf("failed to read directory '%s': %w", pattern, err)
		}
		paths := make([]string, 0, len(dirEntries))
		for _, dirEntry := range dirEntries {
			paths = append(paths, filepath.Join(pattern, dirEntry.Name()))
		}
		return paths, nil
	}

	return entries, nil
}

func clean(dir string) error {
	dirLog := logrus.WithField("dir", dir)

	paths, err := resolve(dir)
	if err != nil {
		return err
	}

	now := time.Now()
	cntDeleted := 0
	for _, entry := range paths {
		stat, err := os.Stat(entry)
		if err != nil {
			dirLog.Errorf("Failed to stat %s", entry)
			continue
		}
		if stat.Mode().IsRegular() && stat.ModTime().Add(deleteAfter).Before(now) {
			if err := os.Remove(entry); err != nil {
				dirLog.Errorf("Failed to delete file %s", entry)
			} else {
				cntDeleted++
			}
		}
	}

	dirLog.Infof("Cleanup successful, %d files deleted", cntDeleted)
	return nil
}

func env(key string, defaultValue string) string {
	if v, ok := os.LookupEnv(key); ok {
		return v
	}
	return defaultValue
}

func main() {
	var err error
	if deleteAfter, err = time.ParseDuration(env("DELETE_AFTER", "30m")); err != nil {
		logrus.Fatalf("Invalid DELETE_AFTER provided. See the 'https://golang.org/pkg/time/#ParseDuration'")
	}
	if sleep, err = time.ParseDuration(env("SLEEP", "30m")); err != nil {
		logrus.Fatalf("Invalid RUN_EVERY provided. See the 'https://golang.org/pkg/time/#ParseDuration'")
	}
	directories = strings.Split(env("DIRS", ""), ",")
	logrus.Infof("Running the cleanup every %v", sleep)
	logrus.Infof("Deleting files that were changed %v ago", deleteAfter)

	c := make(chan os.Signal, 1)
	signal.Notify(c, os.Interrupt, syscall.SIGTERM, syscall.SIGINT)

	run := func() {
		for _, d := range directories {
			d := strings.TrimSpace(d)
			if err := clean(d); err != nil {
				logrus.Fatalf("Failed to clean directory '%s': %v", d, err)
			}
		}
	}

	ticker := time.NewTicker(sleep)
	defer ticker.Stop()

	run()
loop:
	for {
		select {
		case <-ticker.C:
			run()
		case <-c:
			logrus.Infof("Shutting down")
			break loop
		}
	}
}
