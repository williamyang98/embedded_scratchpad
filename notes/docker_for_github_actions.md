- Linux: ```act --pull=false --artifact-server-path=/tmp/gh-artifacts```
- Windows: ```act --pull=false --artifact-server-path=/tmp/gh-artifacts --network=default```

Extra arguments that are useful:
- Specify workflow: ```act <args> --workflows .github/workflows/*.yml```
- Specify job: ```act <args> --jobs build```


Run docker container from act image: ```docker run --interactive --tty <image_name>``` or ```docker run -it <image_name>```
- Both mean the same thing ```-it``` just combines ```---interactive``` and ```--tty``` for console 