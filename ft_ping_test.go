package ft_ping

import (
	"os/exec"
	"testing"
)

type arguments struct {
	args []string
}

var successTests = []arguments{
	{args: []string{"google.com", "-c 1"}},
	{args: []string{"google.com", "-c 2"}},
	{args: []string{"google.com", "-c 1", "-t 3"}},
	{args: []string{"google.com", "-c 1", "-t 12"}},
	{args: []string{"google.com", "-c 1", "-t 45"}},
	{args: []string{"google.com", "-t 12", "-c 2"}},
	{args: []string{"-v ", "-c 1", "192.168.1.1"}},
	{args: []string{"-v ", "192.168.1.1", "-c 1"}},
	{args: []string{"-v ", "192.168.0.1", "-c 1"}},
}

var failureTests = []arguments{
	{args: []string{"unknown.host", "-c 1"}},
	{args: []string{"-c ttata"}},
	{args: []string{"-t 256"}},
}

func TestSuccess(t *testing.T) {
	for _, test := range successTests {
		out, err := exec.Command("./ft_ping", test.args...).Output()
		t.Logf("args: %s\n%s", test.args, out)
		if err != nil {
			t.Error(err)
		}
	}
}

func TestFailure(t *testing.T) {
	for _, test := range failureTests {
		err := exec.Command("./ft_ping", test.args...).Run()
		t.Logf("args: %s : %s", test.args, err.Error())
		if err == nil {
			t.Error(err)
		}
	}
}
